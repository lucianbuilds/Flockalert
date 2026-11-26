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

  int gpsRxPin;
  int gpsTxPin;

  int screenSdaPin;
  int screenSclPin;

  int speakerPin;

  uint32_t gpsBaud;

  double alertDistanceFeet;
  double alertClearDistanceFeet;

  double highwaySpeedMph;
  double highwayAlertDistanceFeet;

  uint32_t displayUpdateMs;

  int coordinateDecimals;
  int speedTextSize;

  int minSatellites;

  bool autoScreenSwitch;

  bool buzzerEnabled;
  int chimeTone;
};


// ============================================================
// GLOBALS FROM GPS_ALERT.INO
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
static unsigned long bootHoldStart = 0;

static bool bootHoldTriggered = false;


// ============================================================
// LOAD SETTINGS
// ============================================================

void loadPersistentConfig() {

  // ----------------------------------------------------------
  // Defaults
  // ----------------------------------------------------------

  runtimeSettings.gpsRxPin =
    DEFAULT_GPS_RX_PIN;

  runtimeSettings.gpsTxPin =
    DEFAULT_GPS_TX_PIN;

  runtimeSettings.screenSdaPin =
    DEFAULT_SCREEN_SDA_PIN;

  runtimeSettings.screenSclPin =
    DEFAULT_SCREEN_SCL_PIN;

  runtimeSettings.speakerPin =
    DEFAULT_SPEAKER_PIN;

  runtimeSettings.gpsBaud =
    DEFAULT_GPS_BAUD;

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

  runtimeSettings.autoScreenSwitch =
    DEFAULT_AUTO_SCREEN_SWITCH;

  runtimeSettings.buzzerEnabled =
    DEFAULT_BUZZER_ENABLED;

  runtimeSettings.chimeTone =
    DEFAULT_CHIME_TONE;

  runtimeSettings.highwaySpeedMph =
  DEFAULT_HIGHWAY_SPEED_MPH;

  runtimeSettings.highwayAlertDistanceFeet =
    DEFAULT_HIGHWAY_ALERT_DISTANCE_FEET;

  alprs_passed = 0;


  // ----------------------------------------------------------
  // Open NVS
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
  // Load saved values
  // ----------------------------------------------------------

  runtimeSettings.gpsRxPin =
    settingsPreferences.getInt(
      "gpsrx",
      DEFAULT_GPS_RX_PIN
    );

  runtimeSettings.gpsTxPin =
    settingsPreferences.getInt(
      "gpstx",
      DEFAULT_GPS_TX_PIN
    );

  runtimeSettings.screenSdaPin =
    settingsPreferences.getInt(
      "sda",
      DEFAULT_SCREEN_SDA_PIN
    );

  runtimeSettings.screenSclPin =
    settingsPreferences.getInt(
      "scl",
      DEFAULT_SCREEN_SCL_PIN
    );

  runtimeSettings.speakerPin =
    settingsPreferences.getInt(
      "speaker",
      DEFAULT_SPEAKER_PIN
    );

  runtimeSettings.gpsBaud =
    settingsPreferences.getUInt(
      "gpsbaud",
      DEFAULT_GPS_BAUD
    );

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

  runtimeSettings.highwaySpeedMph =
    settingsPreferences.getDouble(
      "hwyspeed",
      DEFAULT_HIGHWAY_SPEED_MPH
    );

  runtimeSettings.highwayAlertDistanceFeet =
    settingsPreferences.getDouble(
      "hwyft",
      DEFAULT_HIGHWAY_ALERT_DISTANCE_FEET
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

  runtimeSettings.autoScreenSwitch =
    settingsPreferences.getBool(
      "autosw",
      DEFAULT_AUTO_SCREEN_SWITCH
    );

  runtimeSettings.buzzerEnabled =
    settingsPreferences.getBool(
      "buzzen",
      DEFAULT_BUZZER_ENABLED
    );

  runtimeSettings.chimeTone =
    settingsPreferences.getInt(
      "chime",
      DEFAULT_CHIME_TONE
    );

  if (runtimeSettings.chimeTone < 0 ||
      runtimeSettings.chimeTone >= CHIME_TONE_COUNT) {

    runtimeSettings.chimeTone =
      DEFAULT_CHIME_TONE;
  }

  alprs_passed =
    settingsPreferences.getUInt(
      "passed",
      0
    );


  // ----------------------------------------------------------
  // Basic validation
  // ----------------------------------------------------------

  if (runtimeSettings.gpsRxPin < 0 ||
      runtimeSettings.gpsRxPin > 39) {

    runtimeSettings.gpsRxPin =
      DEFAULT_GPS_RX_PIN;
  }

  if (runtimeSettings.gpsTxPin < -1 ||
      runtimeSettings.gpsTxPin > 39) {

    runtimeSettings.gpsTxPin =
      DEFAULT_GPS_TX_PIN;
  }

  if (runtimeSettings.screenSdaPin < 0 ||
      runtimeSettings.screenSdaPin > 39) {

    runtimeSettings.screenSdaPin =
      DEFAULT_SCREEN_SDA_PIN;
  }

  if (runtimeSettings.screenSclPin < 0 ||
      runtimeSettings.screenSclPin > 39) {

    runtimeSettings.screenSclPin =
      DEFAULT_SCREEN_SCL_PIN;
  }

  if (runtimeSettings.speakerPin < 0 ||
      runtimeSettings.speakerPin > 39) {

    runtimeSettings.speakerPin =
      DEFAULT_SPEAKER_PIN;
  }

  if (runtimeSettings.alertDistanceFeet < 1 ||
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

  if (runtimeSettings.gpsBaud != 4800 &&
      runtimeSettings.gpsBaud != 9600 &&
      runtimeSettings.gpsBaud != 19200 &&
      runtimeSettings.gpsBaud != 38400 &&
      runtimeSettings.gpsBaud != 57600 &&
      runtimeSettings.gpsBaud != 115200) {

    runtimeSettings.gpsBaud =
      DEFAULT_GPS_BAUD;
  }

  if (runtimeSettings.highwaySpeedMph <= 0 ||
    runtimeSettings.highwaySpeedMph > 200) {

  runtimeSettings.highwaySpeedMph =
    DEFAULT_HIGHWAY_SPEED_MPH;
  }

  if (runtimeSettings.highwayAlertDistanceFeet < 1 ||
    runtimeSettings.highwayAlertDistanceFeet > 5000) {

  runtimeSettings.highwayAlertDistanceFeet =
    DEFAULT_HIGHWAY_ALERT_DISTANCE_FEET;
  }


  Serial.println(
    "Persistent settings loaded."
  );
}


// ============================================================
// SAVE SETTINGS
// ============================================================

void savePersistentConfig() {

  settingsPreferences.putInt(
    "gpsrx",
    runtimeSettings.gpsRxPin
  );

  settingsPreferences.putInt(
    "gpstx",
    runtimeSettings.gpsTxPin
  );

  settingsPreferences.putInt(
    "sda",
    runtimeSettings.screenSdaPin
  );

  settingsPreferences.putInt(
    "scl",
    runtimeSettings.screenSclPin
  );

  settingsPreferences.putInt(
    "speaker",
    runtimeSettings.speakerPin
  );

  settingsPreferences.putUInt(
    "gpsbaud",
    runtimeSettings.gpsBaud
  );

  settingsPreferences.putDouble(
    "alertft",
    runtimeSettings.alertDistanceFeet
  );

  settingsPreferences.putDouble(
    "clearft",
    runtimeSettings.alertClearDistanceFeet
  );

  settingsPreferences.putDouble(
    "hwyspeed",
    runtimeSettings.highwaySpeedMph
  );

  settingsPreferences.putDouble(
    "hwyft",
    runtimeSettings.highwayAlertDistanceFeet
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

  settingsPreferences.putBool(
    "autosw",
    runtimeSettings.autoScreenSwitch
  );

  settingsPreferences.putBool(
    "buzzen",
    runtimeSettings.buzzerEnabled
  );

  settingsPreferences.putInt(
    "chime",
    runtimeSettings.chimeTone
  );


  Serial.println(
    "Settings saved."
  );
}


// ============================================================
// ALPR COUNTER
// ============================================================

void saveAlprCount() {

  settingsPreferences.putUInt(
    "passed",
    alprs_passed
  );
}


void startChime();


void noteAlprPassed() {

  alprs_passed++;

  saveAlprCount();

  startChime();

  Serial.print(
    "ALPR passed. Total: "
  );

  Serial.println(
    alprs_passed
  );
}


// ============================================================
// CONFIG PAGE
// ============================================================

String makeConfigPage() {

  String page;

  page.reserve(10000);


  page += F(
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' "
    "content='width=device-width,initial-scale=1'>"
    "<title>GPS Alert Configuration</title>"
    "<style>"
    "body{font-family:Arial;margin:20px;"
    "max-width:600px;background:#f5f5f5;}"
    ".card{background:white;padding:15px;"
    "border-radius:10px;margin-bottom:15px;}"
    "input,select{width:100%;padding:10px;"
    "margin:5px 0 15px;box-sizing:border-box;"
    "font-size:16px;}"
    "button{padding:12px 18px;margin:5px 0;"
    "font-size:16px;}"
    ".danger{background:#eee;}"
    ".chk{display:flex;align-items:center;gap:8px;}"
    ".chk input{width:auto;margin:0;}"
    "</style>"
    "</head>"
    "<body>"
  );


  page += F(
    "<h2>GPS Alert Configuration</h2>"
  );


  // ----------------------------------------------------------
  // WIFI INFO
  // ----------------------------------------------------------

  page += F(
    "<div class='card'>"
    "<b>Wi-Fi Configuration</b><br>"
    "SSID: "
  );

  page += CONFIG_AP_SSID;

  page += F(
    "<br>Password: "
  );

  page += CONFIG_AP_PASSWORD;

  page += F(
    "<br>Address: http://192.168.4.1"
    "</div>"
  );


  // ----------------------------------------------------------
  // ALPR COUNTER
  // ----------------------------------------------------------

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


  // ----------------------------------------------------------
  // FORM
  // ----------------------------------------------------------

  page += F(
    "<form action='/save' method='POST'>"
  );


  // ----------------------------------------------------------
  // PIN SETTINGS
  // ----------------------------------------------------------

  page += F(
    "<div class='card'>"
    "<h3>Pin Configuration</h3>"

    "<label>GPS RX pin</label>"
    "<input name='gpsrx' type='number' "
    "min='0' max='39' value='"
  );

  page += String(
    runtimeSettings.gpsRxPin
  );

  page += F(
    "'>"

    "<label>GPS TX pin (-1 = unused)</label>"
    "<input name='gpstx' type='number' "
    "min='-1' max='39' value='"
  );

  page += String(
    runtimeSettings.gpsTxPin
  );

  page += F(
    "'>"

    "<label>Screen SDA pin</label>"
    "<input name='sda' type='number' "
    "min='0' max='39' value='"
  );

  page += String(
    runtimeSettings.screenSdaPin
  );

  page += F(
    "'>"

    "<label>Screen SCL pin</label>"
    "<input name='scl' type='number' "
    "min='0' max='39' value='"
  );

  page += String(
    runtimeSettings.screenSclPin
  );

  page += F(
    "'>"

    "<label>Buzzer pin (speaker)</label>"
    "<input name='speaker' type='number' "
    "min='0' max='39' value='"
  );

  page += String(
    runtimeSettings.speakerPin
  );

  page += F(
    "'>"
    "</div>"
  );


  // ----------------------------------------------------------
  // GPS SETTINGS
  // ----------------------------------------------------------

  page += F(
    "<div class='card'>"
    "<h3>GPS Settings</h3>"

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

    if (runtimeSettings.gpsBaud == baud) {
      page += "' selected>";
    } else {
      page += "'>";
    }

    page += String(baud);
    page += "</option>";
  }


  page += F(
    "</select>"

    "<label>Minimum satellites</label>"
    "<input name='sats' type='number' "
    "min='0' max='30' value='"
  );

  page += String(
    runtimeSettings.minSatellites
  );

  page += F(
    "'>"
    "</div>"
  );


  // ----------------------------------------------------------
  // ALERT SETTINGS
  // ----------------------------------------------------------

  page += F(
    "<div class='card'>"
    "<h3>Alert Settings</h3>"

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

    "<label>Highway mode speed (MPH)</label>"
    "<input name='hwyspeed' type='number' "
    "step='0.1' min='1' max='200' value='"
  );

  page += String(
    runtimeSettings.highwaySpeedMph,
    1
  );

  page += F(
    "'>"

    "<label>Highway alert distance (feet)</label>"
    "<input name='hwyalert' type='number' "
    "step='0.1' min='1' max='5000' value='"
  );

  page += String(
    runtimeSettings.highwayAlertDistanceFeet,
    1
  );

  page += F(
    "'>"
    "</div>"
  );


  // ----------------------------------------------------------
  // DISPLAY SETTINGS
  // ----------------------------------------------------------

  page += F(
    "<div class='card'>"
    "<h3>Display Settings</h3>"

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

    "<label class='chk'>"
    "<input type='checkbox' name='autosw' value='1'"
  );

  if (runtimeSettings.autoScreenSwitch) {
    page += " checked";
  }

  page += F(
    ">"
    " Switch to full-screen alert (off = show alert bar)"
    "</label>"
    "</div>"
  );


  // ----------------------------------------------------------
  // SAVE
  // ----------------------------------------------------------

  // ----------------------------------------------------------
  // BUZZER SETTINGS
  // ----------------------------------------------------------

  page += F(
    "<div class='card'>"
    "<h3>Buzzer</h3>"
    "<label class='chk'>"
    "<input type='checkbox' name='buzz' value='1'"
  );

  if (runtimeSettings.buzzerEnabled) {
    page += " checked";
  }

  page += F(
    ">"
    " Play a chime when an ALPR is nearby"
    "</label>"

    "<label>Chime tone</label>"
    "<select name='chime'>"
  );

  for (int i = 0; i < CHIME_TONE_COUNT; i++) {

    page += "<option value='";
    page += String(i);

    if (runtimeSettings.chimeTone == i) {
      page += "' selected>";
    } else {
      page += "'>";
    }

    page += CHIME_TONE_NAMES[i];
    page += "</option>";
  }

  page += F(
    "</select>"
    "</div>"
  );


  page += F(
    "<button type='submit'>"
    "Save & Restart"
    "</button>"
    "</form>"
  );


  // ----------------------------------------------------------
  // DEFAULTS
  // ----------------------------------------------------------

  page += F(
    "<div class='card'>"
    "<h3>Reset</h3>"
    "<form action='/defaults' method='POST'>"
    "<button class='danger' type='submit'>"
    "Reset to Defaults"
    "</button>"
    "</form>"
    "</div>"
  );


  page += F(
    "</body>"
    "</html>"
  );


  return page;
}


// ============================================================
// ROOT
// ============================================================

void handleConfigRoot() {

  configServer.send(
    200,
    "text/html",
    makeConfigPage()
  );
}


// ============================================================
// SAVE
// ============================================================

void handleConfigSave() {

  // ----------------------------------------------------------
  // Required arguments
  // ----------------------------------------------------------

  const char* required[] = {
    "gpsrx",
    "gpstx",
    "sda",
    "scl",
    "speaker",
    "baud",
    "sats",
    "alert",
    "clear",
    "hwyspeed",
    "hwyalert",
    "disp",
    "coord",
    "speed"
  };


  for (
    const char* arg : required
  ) {

    if (!configServer.hasArg(arg)) {

      configServer.send(
        400,
        "text/plain",
        "Missing configuration value."
      );

      return;
    }
  }


  // ----------------------------------------------------------
  // Parse values
  // ----------------------------------------------------------

  int gpsRx =
    configServer.arg(
      "gpsrx"
    ).toInt();

  int gpsTx =
    configServer.arg(
      "gpstx"
    ).toInt();

  int sda =
    configServer.arg(
      "sda"
    ).toInt();

  int scl =
    configServer.arg(
      "scl"
    ).toInt();

  int speaker =
    configServer.arg(
      "speaker"
    ).toInt();

  uint32_t baud =
    configServer.arg(
      "baud"
    ).toInt();

  int sats =
    configServer.arg(
      "sats"
    ).toInt();

  double alert =
    configServer.arg(
      "alert"
    ).toDouble();

  double clear =
    configServer.arg(
      "clear"
    ).toDouble();

  double highwaySpeed =
    configServer.arg(
      "hwyspeed"
    ).toDouble();

  double highwayAlert =
    configServer.arg(
      "hwyalert"
    ).toDouble();

  uint32_t disp =
    configServer.arg(
      "disp"
    ).toInt();

  int coord =
    configServer.arg(
      "coord"
    ).toInt();

  int speed =
    configServer.arg(
      "speed"
    ).toInt();


  // ----------------------------------------------------------
  // Validate GPIOs
  // ----------------------------------------------------------

  if (gpsRx < 0 || gpsRx > 39 ||
      gpsTx < -1 || gpsTx > 39 ||
      sda < 0 || sda > 39 ||
      scl < 0 || scl > 39 ||
      speaker < 0 || speaker > 39) {

    configServer.send(
      400,
      "text/plain",
      "Invalid GPIO pin."
    );

    return;
  }


  // ----------------------------------------------------------
  // Prevent duplicate critical pins
  // ----------------------------------------------------------

  if (gpsRx == sda ||
      gpsRx == scl ||
      gpsRx == speaker ||
      (gpsTx >= 0 && (
        gpsTx == sda ||
        gpsTx == scl ||
        gpsTx == speaker
      )) ||
      sda == scl ||
      sda == speaker ||
      scl == speaker) {

    configServer.send(
      400,
      "text/plain",
      "Pin conflict detected. "
      "Each configured function needs its own GPIO."
    );

    return;
  }


  // ----------------------------------------------------------
  // Validate GPS baud
  // ----------------------------------------------------------

  if (baud != 4800 &&
      baud != 9600 &&
      baud != 19200 &&
      baud != 38400 &&
      baud != 57600 &&
      baud != 115200) {

    configServer.send(
      400,
      "text/plain",
      "Invalid GPS baud rate."
    );

    return;
  }


  // ----------------------------------------------------------
  // Validate remaining settings
  // ----------------------------------------------------------

  if (sats < 0 || sats > 30) {

    configServer.send(
      400,
      "text/plain",
      "Invalid satellite setting."
    );

    return;
  }


  if (alert < 1 ||
      alert > 5000) {

    configServer.send(
      400,
      "text/plain",
      "Invalid alert distance."
    );

    return;
  }


  if (clear <= alert ||
      clear > 10000) {

    configServer.send(
      400,
      "text/plain",
      "Clear distance must be greater than alert distance."
    );

    return;
  }


  if (highwaySpeed <= 0 ||
      highwaySpeed > 200) {

    configServer.send(
      400,
      "text/plain",
      "Invalid highway speed."
    );

    return;
  }


  if (highwayAlert < 1 ||
      highwayAlert > 5000) {

    configServer.send(
      400,
      "text/plain",
      "Invalid highway alert distance."
    );

    return;
  }


  if (disp < 50 ||
      disp > 2000) {

    configServer.send(
      400,
      "text/plain",
      "Invalid display interval."
    );

    return;
  }


  if (coord < 3 ||
      coord > 6) {

    configServer.send(
      400,
      "text/plain",
      "Invalid coordinate precision."
    );

    return;
  }


  int chime =
    configServer.arg(
      "chime"
    ).toInt();

  if (chime < 0 ||
      chime >= CHIME_TONE_COUNT) {

    configServer.send(
      400,
      "text/plain",
      "Invalid chime tone."
    );

    return;
  }


  if (speed < 1 ||
      speed > 4) {

    configServer.send(
      400,
      "text/plain",
      "Invalid speed text size."
    );

    return;
  }


  // ----------------------------------------------------------
  // Apply
  // ----------------------------------------------------------

  runtimeSettings.gpsRxPin =
    gpsRx;

  runtimeSettings.gpsTxPin =
    gpsTx;

  runtimeSettings.screenSdaPin =
    sda;

  runtimeSettings.screenSclPin =
    scl;

  runtimeSettings.speakerPin =
    speaker;

  runtimeSettings.gpsBaud =
    baud;

  runtimeSettings.minSatellites =
    sats;

  runtimeSettings.alertDistanceFeet =
    alert;

  runtimeSettings.alertClearDistanceFeet =
    clear;

  runtimeSettings.highwaySpeedMph =
    highwaySpeed;

  runtimeSettings.highwayAlertDistanceFeet =
    highwayAlert;

  runtimeSettings.displayUpdateMs =
    disp;

  runtimeSettings.coordinateDecimals =
    coord;

  runtimeSettings.speedTextSize =
    speed;

  // Unchecked checkboxes are not sent by the browser, so absence = off.
  runtimeSettings.autoScreenSwitch =
    configServer.hasArg("autosw");

  runtimeSettings.buzzerEnabled =
    configServer.hasArg("buzz");

  runtimeSettings.chimeTone =
    chime;


  // ----------------------------------------------------------
  // Save
  // ----------------------------------------------------------

  savePersistentConfig();


  // ----------------------------------------------------------
  // Restart
  // ----------------------------------------------------------

  configServer.send(
    200,
    "text/html",
    "<html><body>"
    "<h2>Settings saved.</h2>"
    "<p>ESP32 restarting...</p>"
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
// RESET TO DEFAULTS
// ============================================================

void handleRestoreDefaults() {

  runtimeSettings.gpsRxPin =
    DEFAULT_GPS_RX_PIN;

  runtimeSettings.gpsTxPin =
    DEFAULT_GPS_TX_PIN;

  runtimeSettings.screenSdaPin =
    DEFAULT_SCREEN_SDA_PIN;

  runtimeSettings.screenSclPin =
    DEFAULT_SCREEN_SCL_PIN;

  runtimeSettings.speakerPin =
    DEFAULT_SPEAKER_PIN;

  runtimeSettings.gpsBaud =
    DEFAULT_GPS_BAUD;

  runtimeSettings.alertDistanceFeet =
    DEFAULT_ALERT_DISTANCE_FEET;

  runtimeSettings.alertClearDistanceFeet =
    DEFAULT_ALERT_CLEAR_DISTANCE_FEET;

  runtimeSettings.highwaySpeedMph =
    DEFAULT_HIGHWAY_SPEED_MPH;

  runtimeSettings.highwayAlertDistanceFeet =
    DEFAULT_HIGHWAY_ALERT_DISTANCE_FEET;

  runtimeSettings.displayUpdateMs =
    DEFAULT_DISPLAY_UPDATE_MS;

  runtimeSettings.coordinateDecimals =
    DEFAULT_COORDINATE_DECIMALS;

  runtimeSettings.speedTextSize =
    DEFAULT_SPEED_TEXT_SIZE;

  runtimeSettings.minSatellites =
    DEFAULT_MIN_SATELLITES;

  runtimeSettings.autoScreenSwitch =
    DEFAULT_AUTO_SCREEN_SWITCH;

  runtimeSettings.buzzerEnabled =
    DEFAULT_BUZZER_ENABLED;

  runtimeSettings.chimeTone =
    DEFAULT_CHIME_TONE;


  savePersistentConfig();


  configServer.send(
    200,
    "text/html",
    "<html><body>"
    "<h2>Defaults restored.</h2>"
    "<p>ESP32 restarting...</p>"
    "</body></html>"
  );


  delay(750);

  ESP.restart();
}


// ============================================================
// START CONFIGURATION AP
// ============================================================

void startConfigPortal() {

  if (configPortalActive) {
    return;
  }


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "   CONFIGURATION MODE ENABLED"
  );

  Serial.println(
    "================================"
  );


  WiFi.mode(
    WIFI_AP
  );


  bool started =
    WiFi.softAP(
      CONFIG_AP_SSID,
      CONFIG_AP_PASSWORD,
      CONFIG_AP_CHANNEL,
      false,
      CONFIG_AP_MAX_CONNECTIONS
    );


  if (!started) {

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


  configServer.onNotFound(
    []() {

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
    }
  );


  configServer.begin();


  configPortalActive = true;


  Serial.print(
    "SSID: "
  );

  Serial.println(
    CONFIG_AP_SSID
  );


  Serial.print(
    "Password: "
  );

  Serial.println(
    CONFIG_AP_PASSWORD
  );


  Serial.print(
    "Address: http://"
  );

  Serial.println(
    WiFi.softAPIP()
  );
}


// ============================================================
// BOOT BUTTON
// ============================================================

void checkBootButton() {

  if (configPortalActive) {
    return;
  }


  bool rawState =
    digitalRead(
      BOOT_BUTTON_PIN
    );


  // ----------------------------------------------------------
  // Debounce
  // ----------------------------------------------------------

  if (
    rawState !=
    bootButtonLastRaw
  ) {

    bootButtonLastChange =
      millis();

    bootButtonLastRaw =
      rawState;
  }


  if (
    millis() -
    bootButtonLastChange <
    BOOT_DEBOUNCE_MS
  ) {

    return;
  }


  // ----------------------------------------------------------
  // Stable transition
  // ----------------------------------------------------------

  if (
    rawState !=
    bootButtonStable
  ) {

    bootButtonStable =
      rawState;


    // --------------------------------------------------------
    // BUTTON PRESSED
    // --------------------------------------------------------

    if (
      bootButtonStable == LOW
    ) {

      bootHoldStart =
        millis();

      bootHoldTriggered =
        false;


      Serial.println(
        "BOOT button pressed."
      );
    }


    // --------------------------------------------------------
    // BUTTON RELEASED
    // --------------------------------------------------------

    else {

      bootHoldStart =
        0;

      bootHoldTriggered =
        false;


      Serial.println(
        "BOOT button released."
      );
    }
  }


  // ----------------------------------------------------------
  // HELD FOR 3 SECONDS
  // ----------------------------------------------------------

  if (
    bootButtonStable == LOW &&
    bootHoldStart != 0 &&
    !bootHoldTriggered &&
    millis() - bootHoldStart >=
      BOOT_HOLD_TIME_MS
  ) {

    bootHoldTriggered =
      true;

    Serial.println(
      "BOOT held for 3 seconds."
    );

    startConfigPortal();
  }
}


// ============================================================
// CONFIG SERVER LOOP
// ============================================================

void handleConfigPortal() {

  if (!configPortalActive) {
    return;
  }

  configServer.handleClient();
}

#endif
