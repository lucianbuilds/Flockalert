#ifndef DOWNLOAD_H
#define DOWNLOAD_H

// ============================================================
// CAMERA DOWNLOAD OVER WI-FI
// ============================================================
//
// From the config portal, the device joins a Wi-Fi network (e.g. a
// phone hotspot) while keeping its own access point up, asks the
// Overpass API for ALPRs and speed cameras around a city, and saves
// the result as the camera database (same file as a manual upload).
//
// The download runs from loop(), not inside the web request, because
// joining another network can move the access point to a new channel
// and briefly drop the phone's connection to the portal.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include "cameras.h"

extern Preferences settingsPreferences;

bool cameraDownloadPending = false;
String cameraDownloadStatus = "";

const char* OVERPASS_SERVERS[] = {
  "https://overpass-api.de/api/interpreter",
  "https://maps.mail.ru/osm/tools/overpass/api/interpreter"
};


// ============================================================
// FIRST-BOOT SETUP FLAG
// ============================================================

bool setupComplete() {
  return settingsPreferences.getBool("setupdone", false);
}

void markSetupComplete() {
  settingsPreferences.putBool("setupdone", true);
}


// ============================================================
// HELPERS
// ============================================================

// Keeps only characters that are safe inside an Overpass regex.
static String cleanPlaceName(String s) {
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (isalnum((unsigned char)c) || c == ' ' || c == '-' ||
        c == '\'' || (unsigned char)c >= 0x80) {
      out += c;
    }
  }
  out.trim();
  return out;
}

static String urlEncode(const String& s) {
  static const char* hex = "0123456789ABCDEF";
  String out;
  out.reserve(s.length() * 2);
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}

static String htmlEscape(const String& s) {
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '&') out += "&amp;";
    else if (c == '\'') out += "&#39;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

// Same output columns as tools/overpass/cameras.overpassql.
static String buildOverpassQuery(
  const String& city,
  const String& state,
  int radiusMeters
) {
  String r = String(radiusMeters);
  String dir =
    "direction=is_tag(\"camera:direction\")?t[\"camera:direction\"]:t[\"direction\"]";

  String q =
    "[out:csv(name,lat,lon,direction,maxspeed;true;\"\\t\")][timeout:180];";

  String areaFilter = "";

  if (state.length()) {
    q += "area[\"name\"~\"^" + state + "$\",i][\"admin_level\"=\"4\"]->.st;";
    areaFilter = "(area.st)";
  }

  q += "node[\"place\"~\"^(city|town|village|hamlet|suburb)$\"]"
       "[\"name\"~\"^" + city + "$\",i]" + areaFilter + "->.c;";

  q += "node[\"surveillance:type\"=\"ALPR\"](around.c:" + r + ")->.alpr;";

  q += "(node[\"highway\"=\"speed_camera\"](around.c:" + r + ");"
       "rel[\"type\"=\"enforcement\"][\"enforcement\"=\"maxspeed\"](around.c:" + r + ");"
       "node(r:\"device\");)->.sa;";

  q += "(node.sa; - node.alpr;)->.sp;";

  q += ".alpr convert row name=(is_tag(\"manufacturer\")?t[\"manufacturer\"]:\"Unknown\")+\" ALPR\","
       "lat=lat(),lon=lon()," + dir + ";out;";

  q += ".sp convert row name=\"Speed Camera\",lat=lat(),lon=lon()," + dir +
       ",maxspeed=t[\"maxspeed\"];out;";

  return q;
}


// ============================================================
// DOWNLOAD
// ============================================================

static bool joinWifi(const String& ssid, const String& pass) {

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 20000) {
    delay(250);
  }

  return WiFi.status() == WL_CONNECTED;
}

// Streams one server's response into CAMERA_TEMP_FILE.
static int fetchFromServer(const char* url, const String& body) {

  WiFiClientSecure client;
  client.setInsecure();   // Public map data; no certificate store on board

  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(65000);

  if (!http.begin(client, url)) return -1;

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setUserAgent("FlockAlert-ESP32");

  int code = http.POST(body);

  if (code == 200) {
    File f = LittleFS.open(CAMERA_TEMP_FILE, "w");
    if (!f) {
      http.end();
      return -2;
    }
    int written = http.writeToStream(&f);
    f.close();
    if (written <= 0) code = -3;

    // A busy server can answer 200 with an HTML error page instead of
    // the CSV, which always starts with the "name" header.
    File check = LittleFS.open(CAMERA_TEMP_FILE, "r");
    char head[5] = "";
    if (check) {
      check.readBytes(head, 4);
      check.close();
    }
    if (code == 200 && strncmp(head, "name", 4) != 0) code = -4;
  }

  http.end();
  return code;
}

void runCameraDownload() {

  String ssid = settingsPreferences.getString("dlssid", "");
  String pass = settingsPreferences.getString("dlpass", "");
  String city = settingsPreferences.getString("dlcity", "");
  String state = settingsPreferences.getString("dlstate", "");
  int radiusMi = settingsPreferences.getInt("dlrad", 10);

  Serial.printf("Downloading cameras near %s, %s (%d mi)\n",
    city.c_str(), state.c_str(), radiusMi);

  if (!joinWifi(ssid, pass)) {
    cameraDownloadStatus =
      "Could not join Wi-Fi \"" + ssid + "\". Check the name and password.";
    WiFi.disconnect(false);
    WiFi.mode(WIFI_AP);
    return;
  }

  String body =
    "data=" + urlEncode(buildOverpassQuery(city, state, radiusMi * 1609));

  int code = -1;

  for (const char* url : OVERPASS_SERVERS) {
    code = fetchFromServer(url, body);
    Serial.printf("Overpass %s -> %d\n", url, code);
    if (code == 200) break;
  }

  WiFi.disconnect(false);
  WiFi.mode(WIFI_AP);

  if (code != 200) {
    LittleFS.remove(CAMERA_TEMP_FILE);
    cameraDownloadStatus =
      "Download failed (code " + String(code) +
      "). The map server may be busy; try again or use a smaller radius.";
    return;
  }

  LittleFS.remove(CAMERA_FILE);
  LittleFS.rename(CAMERA_TEMP_FILE, CAMERA_FILE);
  loadCameraDatabase();

  if (!camerasFromFile) {
    LittleFS.remove(CAMERA_FILE);
    loadCameraDatabase();
    cameraDownloadStatus =
      "No cameras found. Check the city and state spelling, or try a larger radius.";
    return;
  }

  markSetupComplete();

  cameraDownloadStatus =
    "Downloaded " + String(cameraCount) + " cameras near " + city +
    (state.length() ? ", " + state : String("")) + ".";
}

// Called from the portal loop.
void handleCameraDownloadLoop() {
  if (!cameraDownloadPending) return;
  cameraDownloadPending = false;
  runCameraDownload();
}


// ============================================================
// PORTAL
// ============================================================

void handleCameraDownloadRequest() {

  String ssid = configServer.arg("ssid");
  String pass = configServer.arg("pass");
  String city = cleanPlaceName(configServer.arg("city"));
  String state = cleanPlaceName(configServer.arg("state"));
  int radius = configServer.arg("radius").toInt();

  if (!ssid.length() || !city.length() || radius < 1 || radius > 100) {
    configServer.send(400, "text/plain",
      "Enter a Wi-Fi name, a city, and a radius from 1 to 100 miles.");
    return;
  }

  // Keep the last values so the form is prefilled next time.
  // A blank password field keeps the saved one.
  settingsPreferences.putString("dlssid", ssid);
  if (pass.length()) settingsPreferences.putString("dlpass", pass);
  settingsPreferences.putString("dlcity", city);
  settingsPreferences.putString("dlstate", state);
  settingsPreferences.putInt("dlrad", radius);

  cameraDownloadStatus = "Downloading...";
  cameraDownloadPending = true;

  configServer.send(200, "text/html",
    "<html><head><meta name='viewport' content='width=device-width'>"
    "<meta http-equiv='refresh' content='45;url=/'></head><body>"
    "<h3>Downloading cameras</h3>"
    "<p>This takes up to a couple of minutes. Your phone may drop off "
    "the device's Wi-Fi while it connects; rejoin it if needed.</p>"
    "<p>This page returns to settings automatically.</p>"
    "</body></html>");
}

String makeCameraDownloadCard() {

  String page;

  if (!setupComplete()) {
    page += F(
      "<div class='card'><h3>Welcome! Let's set up</h3>"
      "<p>1. Download cameras for your area below.<br>"
      "2. Review the settings and press Save.</p></div>"
    );
  }

  page += F("<div class='card'><h3>Download Cameras</h3>");

  if (cameraDownloadStatus.length()) {
    page += "<p><b>" + htmlEscape(cameraDownloadStatus) + "</b></p>";
  }

  page += F(
    "<form action='/download-cameras' method='POST'>"
    "<label>Internet Wi-Fi name (home Wi-Fi or phone hotspot)</label>"
    "<input name='ssid' required value='"
  );
  page += htmlEscape(settingsPreferences.getString("dlssid", ""));
  page += F(
    "'>"
    "<label>Wi-Fi password (blank = keep saved)</label>"
    "<input name='pass' type='password'>"
    "<label>City</label>"
    "<input name='city' required value='"
  );
  page += htmlEscape(settingsPreferences.getString("dlcity", ""));
  page += F(
    "'>"
    "<label>State / province</label>"
    "<input name='state' value='"
  );
  page += htmlEscape(settingsPreferences.getString("dlstate", ""));
  page += F(
    "'>"
    "<label>Search radius (miles, 1-100)</label>"
    "<input name='radius' type='number' min='1' max='100' value='"
  );
  page += String(settingsPreferences.getInt("dlrad", 10));
  page += F(
    "'>"
    "<button type='submit'>Download Cameras</button>"
    "</form></div>"
  );

  return page;
}

#endif
