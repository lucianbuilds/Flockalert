#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#include "config.h"


// ============================================================
// RUNTIME SETTINGS
// ============================================================

struct RuntimeSettings {

  double alertDistanceFeet;
  double alertClearDistanceFeet;

  uint32_t displayUpdateMs;

  int coordinateDecimals;
  int speedTextSize;

  int minSatellites;

  uint32_t gpsBaud;
};


// ============================================================
// GLOBALS DEFINED IN GPS_ALERT.INO
// ============================================================

extern RuntimeSettings runtimeSettings;

extern uint32_t alprs_passed;

extern bool configPortalActive;

extern WebServer configServer;

extern Preferences settingsPreferences;


// ============================================================
// BOOT BUTTON STATE
// ============================================================

static bool bootButtonLastRaw = HIGH;
static bool bootButtonStable = HIGH;

static unsigned long bootButtonLastChange = 0;

static uint8_t bootPressCount = 0;
static unsigned long firstBootPressTime = 0;


// ============================================================
// LOAD SETTINGS FROM NVS
// ============================================================

void loadPersistentConfig() {

  // Start with compile-time defaults.

  runtimeSettings.alertDistanceFeet =
    DEFAULT_ALERT_DISTANCE_FEET;

  runtimeSettings.alertClearDistanceFeet =
    DEFAULT_ALERT_CLEAR_DISTANCE_FEET;

  runtimeSettings.displayUpdateMs =
    DEFAULT_DISPLAY_UPDATE_MS;

  runtimeSettings.coordinateDecimals =
    DEFAULT_COORDINATE_DECIMALS;

  runtimeSettings.speedTextSize =
    DEFAULT_SPEED_TEXT_SIZE;

  runtimeSettings.minSatellites =
    DEFAULT_MIN_SATELLITES;

  runtimeSettings.gpsBaud =
    DEFAULT_GPS_BAUD;

  alprs_passed = 0;

  // ----------------------------------------------------------
  // Open persistent storage
  // ----------------------------------------------------------

  if (!settingsPreferences.begin(
        "gpsalert",
        false
      )) {

    Serial.println(
      "WARNING: Could not open NVS settings."
    );

    return;
  }

  // ----------------------------------------------------------
  // Read saved settings
  // ----------------------------------------------------------

  runtimeSettings.alertDistanceFeet =
    settingsPreferences.getDouble(
      "alertft",
      DEFAULT_ALERT_DISTANCE_FEET
    );

  runtimeSettings.alertClearDistanceFeet =
    settingsPreferences.getDouble(
      "clearft",
      DEFAULT_ALERT_CLEAR_DISTANCE_FEET
    );

  runtimeSettings.displayUpdateMs =
    settingsPreferences.getUInt(
      "dispms",
      DEFAULT_DISPLAY_UPDATE_MS
    );

  runtimeSettings.coordinateDecimals =
    settingsPreferences.getInt(
      "coorddec",
      DEFAULT_COORDINATE_DECIMALS
    );

  runtimeSettings.speedTextSize =
    settingsPreferences.getInt(
      "speedsz",
      DEFAULT_SPEED_TEXT_SIZE
    );

  runtimeSettings.minSatellites =
    settingsPreferences.getInt(
      "minsats",
      DEFAULT_MIN_SATELLITES
    );

  runtimeSettings.gpsBaud =
    settingsPreferences.getUInt(
      "gpsbaud",
      DEFAULT_GPS_BAUD
    );

  alprs_passed =
    settingsPreferences.getUInt(
      "passed",
      0
    );

  // ----------------------------------------------------------
  // Sanitize settings
  // ----------------------------------------------------------

  if (runtimeSettings.alertDistanceFeet <= 0 ||
      runtimeSettings.alertDistanceFeet > 5000) {

    runtimeSettings.alertDistanceFeet =
      DEFAULT_ALERT_DISTANCE_FEET;
  }

  if (runtimeSettings.alertClearDistanceFeet <=
      runtimeSettings.alertDistanceFeet ||
      runtimeSettings.alertClearDistanceFeet > 10000) {

    runtimeSettings.alertClearDistanceFeet =
      runtimeSettings.alertDistanceFeet + 25.0;
  }

  if (runtimeSettings.displayUpdateMs < 50 ||
      runtimeSettings.displayUpdateMs > 2000) {

    runtimeSettings.displayUpdateMs =
      DEFAULT_DISPLAY_UPDATE_MS;
  }

  if (runtimeSettings.coordinateDecimals < 3 ||
      runtimeSettings.coordinateDecimals > 6) {

    runtimeSettings.coordinateDecimals =
      DEFAULT_COORDINATE_DECIMALS;
  }

  if (runtimeSettings.speedTextSize < 1 ||
      runtimeSettings.speedTextSize > 4) {

    runtimeSettings.speedTextSize =
      DEFAULT_SPEED_TEXT_SIZE;
  }

  if (runtimeSettings.minSatellites < 0 ||
      runtimeSettings.minSatellites > 30) {

    runtimeSettings.minSatellites =
      DEFAULT_MIN_SATELLITES;
  }

  // Only allow common u-blox UART speeds.

  if (runtimeSettings.gpsBaud != 4800 &&
      runtimeSettings.gpsBaud != 9600 &&
      runtimeSettings.gpsBaud != 19200 &&
      runtimeSettings.gpsBaud != 38400 &&
      runtimeSettings.gpsBaud != 57600 &&
      runtimeSettings.gpsBaud != 115200) {

    runtimeSettings.gpsBaud =
      DEFAULT_GPS_BAUD;
  }

  Serial.println("Persistent settings loaded.");
}


// ============================================================
// SAVE SETTINGS
// ============================================================

void savePersistentConfig() {

  settingsPreferences.putDouble(
    "alertft",
    runtimeSettings.alertDistanceFeet
  );

  settingsPreferences.putDouble(
    "clearft",
    runtimeSettings.alertClearDistanceFeet
  );

  settingsPreferences.putUInt(
    "dispms",
    runtimeSettings.displayUpdateMs
  );

  settingsPreferences.putInt(
    "coorddec",
    runtimeSettings.coordinateDecimals
  );

  settingsPreferences.putInt(
    "speedsz",
    runtimeSettings.speedTextSize
  );

  settingsPreferences.putInt(
    "minsats",
    runtimeSettings.minSatellites
  );

  settingsPreferences.putUInt(
    "gpsbaud",
    runtimeSettings.gpsBaud
  );

  Serial.println(
    "Settings saved to NVS."
  );
}


// ============================================================
// SAVE ALPR COUNTER
// ============================================================

void saveAlprCount() {

  settingsPreferences.putUInt(
    "passed",
    alprs_passed
  );
}


// ============================================================
// INCREMENT ALPR COUNTER
// ============================================================

void noteAlprPassed() {

  alprs_passed++;

  saveAlprCount();

  Serial.print(
    "ALPR passed. Total: "
  );

  Serial.println(
    alprs_passed
  );
}


// ============================================================
// CONFIGURATION WEB PAGE
// ============================================================

String makeConfigPage() {

  String page;

  page.reserve(7000);

  page += F(
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' "
    "content='width=device-width,initial-scale=1'>"
    "<title>GPS Alert Configuration</title>"
    "<style>"
    "body{font-family:Arial;margin:20px;max-width:600px;}"
    "input{width:100%;padding:9px;margin:5px 0 15px;"
    "box-sizing:border-box;}"
    "button{padding:12px 18px;margin:5px 0;"
    "font-size:16px;}"
    ".card{padding:15px;border:1px solid #ccc;"
    "border-radius:8px;margin-bottom:15px;}"
    "</style>"
    "</head>"
    "<body>"
  );

  page += F(
    "<h2>GPS Alert Configuration</h2>"
  );

  page += F(
    "<div class='card'>"
    "<b>Configuration Wi-Fi</b><br>"
    "SSID: "
  );

  page += CONFIG_AP_SSID;

  page += F(
    "<br>Address: http://192.168.4.1"
    "</div>"
  );

  page += F(
    "<div class='card'>"
    "<b>ALPRs passed:</b> "
  );

  page += String(
    alprs_passed
  );

  page += F(
    "<form action='/reset-counter' method='POST'>"
    "<button type='submit'>Reset ALPR Counter</button>"
    "</form>"
    "</div>"
  );

  page += F(
    "<form action='/save' method='POST'>"

    "<div class='card'>"

    "<label>Alert distance (feet)</label>"
    "<input name='alert' type='number' "
    "step='0.1' min='1' max='5000' value='"
  );

  page += String(
    runtimeSettings.alertDistanceFeet,
    1
  );

  page += F(
    "'>"

    "<label>Alert clear distance (feet)</label>"
    "<input name='clear' type='number' "
    "step='0.1' min='1' max='10000' value='"
  );

  page += String(
    runtimeSettings.alertClearDistanceFeet,
    1
  );

  page += F(
    "'>"

    "<label>Minimum satellites</label>"
    "<input name='sats' type='number' "
    "min='0' max='30' value='"
  );

  page += String(
    runtimeSettings.minSatellites
  );

  page += F(
    "'>"

    "<label>OLED update interval (ms)</label>"
    "<input name='disp' type='number' "
    "min='50' max='2000' value='"
  );

  page += String(
    runtimeSettings.displayUpdateMs
  );

  page += F(
    "'>"

    "<label>Coordinate decimal places</label>"
    "<input name='coord' type='number' "
    "min='3' max='6' value='"
  );

  page += String(
    runtimeSettings.coordinateDecimals
  );

  page += F(
    "'>"

    "<label>Speed text size (1-4)</label>"
    "<input name='speed' type='number' "
    "min='1' max='4' value='"
  );

  page += String(
    runtimeSettings.speedTextSize
  );

  page += F(
    "'>"

    "<label>GPS baud rate</label>"
    "<select name='baud'>"
  );

  const uint32_t baudRates[] = {
    4800,
    9600,
    19200,
    38400,
    57600,
    115200
  };

  for (uint32_t baud : baudRates) {

    page += "<option value='";
    page += String(baud);
    page += "'";

    if (runtimeSettings.gpsBaud == baud) {
      page += " selected";
    }

    page += ">";
    page += String(baud);
    page += "</option>";
  }

  page += F(
    "</select>"

    "</div>"

    "<button type='submit'>"
    "Save & Restart"
    "</button>"

    "</form>"

    "<div class='card'>"
    "Changing GPS baud requires the ESP32 to restart."
    "</div>"

    "</body>"
    "</html>"
  );

  return page;
}


// ============================================================
// ROOT PAGE
// ============================================================

void handleConfigRoot() {

  configServer.send(
    200,
    "text/html",
    makeConfigPage()
  );
}


// ============================================================
// SAVE CONFIGURATION
// ============================================================

void handleConfigSave() {

  if (!configServer.hasArg("alert") ||
      !configServer.hasArg("clear") ||
      !configServer.hasArg("sats") ||
      !configServer.hasArg("disp") ||
      !configServer.hasArg("coord") ||
      !configServer.hasArg("speed") ||
      !configServer.hasArg("baud")) {

    configServer.send(
      400,
      "text/plain",
      "Missing configuration value."
    );

    return;
  }

  double newAlert =
    configServer.arg("alert").toDouble();

  double newClear =
    configServer.arg("clear").toDouble();

  int newSats =
    configServer.arg("sats").toInt();

  uint32_t newDisplay =
    configServer.arg("disp").toInt();

  int newCoord =
    configServer.arg("coord").toInt();

  int newSpeed =
    configServer.arg("speed").toInt();

  uint32_t newBaud =
    configServer.arg("baud").toInt();

  // ----------------------------------------------------------
  // Validate
  // ----------------------------------------------------------

  if (newAlert < 1 ||
      newAlert > 5000) {

    configServer.send(
      400,
      "text/plain",
      "Invalid alert distance."
    );

    return;
  }

  if (newClear <= newAlert ||
      newClear > 10000) {

    configServer.send(
      400,
      "text/plain",
      "Clear distance must be greater than alert distance."
    );

    return;
  }

  if (newSats < 0 ||
      newSats > 30) {

    configServer.send(
      400,
      "text/plain",
      "Invalid satellite setting."
    );

    return;
  }

  if (newDisplay < 50 ||
      newDisplay > 2000) {

    configServer.send(
      400,
      "text/plain",
      "Invalid display interval."
    );

    return;
  }

  if (newCoord < 3 ||
      newCoord > 6) {

    configServer.send(
      400,
      "text/plain",
      "Invalid coordinate precision."
    );

    return;
  }

  if (newSpeed < 1 ||
      newSpeed > 4) {

    configServer.send(
      400,
      "text/plain",
      "Invalid speed text size."
    );

    return;
  }

  if (newBaud != 4800 &&
      newBaud != 9600 &&
      newBaud != 19200 &&
      newBaud != 38400 &&
      newBaud != 57600 &&
      newBaud != 115200) {

    configServer.send(
      400,
      "text/plain",
      "Invalid GPS baud rate."
    );

    return;
  }

  // ----------------------------------------------------------
  // Apply new values
  // ----------------------------------------------------------

  runtimeSettings.alertDistanceFeet =
    newAlert;

  runtimeSettings.alertClearDistanceFeet =
    newClear;

  runtimeSettings.minSatellites =
    newSats;

  runtimeSettings.displayUpdateMs =
    newDisplay;

  runtimeSettings.coordinateDecimals =
    newCoord;

  runtimeSettings.speedTextSize =
    newSpeed;

  runtimeSettings.gpsBaud =
    newBaud;

  savePersistentConfig();

  // ----------------------------------------------------------
  // Restart
  // ----------------------------------------------------------

  configServer.send(
    200,
    "text/html",
    "<html><body>"
    "<h2>Settings saved.</h2>"
    "<p>Restarting ESP32...</p>"
    "</body></html>"
  );

  delay(750);

  ESP.restart();
}


// ============================================================
// RESET ALPR COUNTER
// ============================================================

void handleResetCounter() {

  alprs_passed = 0;

  saveAlprCount();

  configServer.sendHeader(
    "Location",
    "/",
    true
  );

  configServer.send(
    303,
    "text/plain",
    ""
  );
}


// ============================================================
// RESTORE DEFAULT SETTINGS
// ============================================================

void handleRestoreDefaults() {

  runtimeSettings.alertDistanceFeet =
    DEFAULT_ALERT_DISTANCE_FEET;

  runtimeSettings.alertClearDistanceFeet =
    DEFAULT_ALERT_CLEAR_DISTANCE_FEET;

  runtimeSettings.displayUpdateMs =
    DEFAULT_DISPLAY_UPDATE_MS;

  runtimeSettings.coordinateDecimals =
    DEFAULT_COORDINATE_DECIMALS;

  runtimeSettings.speedTextSize =
    DEFAULT_SPEED_TEXT_SIZE;

  runtimeSettings.minSatellites =
    DEFAULT_MIN_SATELLITES;

  runtimeSettings.gpsBaud =
    DEFAULT_GPS_BAUD;

  savePersistentConfig();

  configServer.send(
    200,
    "text/html",
    "<html><body>"
    "<h2>Defaults restored.</h2>"
    "<p>Restarting ESP32...</p>"
    "</body></html>"
  );

  delay(750);

  ESP.restart();
}


// ============================================================
// START CONFIGURATION ACCESS POINT
// ============================================================

void startConfigPortal() {

  if (configPortalActive) {
    return;
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("   CONFIGURATION MODE ENABLED");
  Serial.println("================================");

  WiFi.mode(WIFI_AP);

  bool apStarted = WiFi.softAP(
    CONFIG_AP_SSID,
    CONFIG_AP_PASSWORD,
    CONFIG_AP_CHANNEL,
    false,
    CONFIG_AP_MAX_CONNECTIONS
  );

  if (!apStarted) {

    Serial.println(
      "ERROR: Failed to start Wi-Fi AP."
    );

    return;
  }

  configServer.on(
    "/",
    HTTP_GET,
    handleConfigRoot
  );

  configServer.on(
    "/save",
    HTTP_POST,
    handleConfigSave
  );

  configServer.on(
    "/reset-counter",
    HTTP_POST,
    handleResetCounter
  );

  configServer.on(
    "/defaults",
    HTTP_POST,
    handleRestoreDefaults
  );

  configServer.onNotFound([]() {

    configServer.sendHeader(
      "Location",
      "/",
      true
    );

    configServer.send(
      302,
      "text/plain",
      ""
    );
  });

  configServer.begin();

  configPortalActive = true;

  Serial.print("Wi-Fi network: ");
  Serial.println(CONFIG_AP_SSID);

  Serial.print("Password: ");
  Serial.println(CONFIG_AP_PASSWORD);

  Serial.print("Configuration address: http://");
  Serial.println(WiFi.softAPIP());

  Serial.println(
    "Configuration server started."
  );
}


// ============================================================
// CHECK BOOT BUTTON
// ============================================================

void checkBootButton() {

  if (configPortalActive) {
    return;
  }

  bool rawState =
    digitalRead(BOOT_BUTTON_PIN);

  // ----------------------------------------------------------
  // Debounce
  // ----------------------------------------------------------

  if (rawState != bootButtonLastRaw) {

    bootButtonLastChange =
      millis();

    bootButtonLastRaw =
      rawState;
  }

  if (millis() -
      bootButtonLastChange <
      BOOT_DEBOUNCE_MS) {

    return;
  }

  // ----------------------------------------------------------
  // Stable state changed
  // ----------------------------------------------------------

  if (rawState != bootButtonStable) {

    bootButtonStable =
      rawState;

    // Button pressed
    if (bootButtonStable == LOW) {

      unsigned long now =
        millis();

      // Start a new triple-press sequence
      // if the previous sequence expired.

      if (bootPressCount == 0 ||
          now - firstBootPressTime >
          CONFIG_TRIPLE_PRESS_WINDOW_MS) {

        bootPressCount = 1;
        firstBootPressTime = now;

      } else {

        bootPressCount++;
      }

      Serial.print(
        "BOOT button press: "
      );

      Serial.println(
        bootPressCount
      );

      // ------------------------------------------------------
      // THREE PRESSES
      // ------------------------------------------------------

      if (bootPressCount >= 3) {

        bootPressCount = 0;

        startConfigPortal();
      }
    }
  }

  // ----------------------------------------------------------
  // Sequence timeout
  // ----------------------------------------------------------

  if (bootPressCount > 0 &&
      millis() - firstBootPressTime >
      CONFIG_TRIPLE_PRESS_WINDOW_MS) {

    bootPressCount = 0;
  }
}


// ============================================================
// WEB SERVER LOOP
// ============================================================

void handleConfigPortal() {

  if (!configPortalActive) {
    return;
  }

  configServer.handleClient();
}

#endif