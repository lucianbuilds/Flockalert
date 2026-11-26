#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>

#include "config.h"
#include "coordinates.h"
#include "cameras.h"
#include "configportal.h"
#include "buzzer.h"


// ============================================================
// GLOBALS USED BY CONFIGPORTAL.H
// ============================================================

RuntimeSettings runtimeSettings;

uint32_t alprs_passed = 0;

uint32_t speedcams_passed = 0;

bool configPortalActive = false;

bool alertStartedInHighwayMode = false;

WebServer configServer(80);

Preferences settingsPreferences;


// ============================================================
// OBJECTS
// ============================================================

TinyGPSPlus gps;

HardwareSerial GPS_Serial(1);

Adafruit_SSD1306 display(
  OLED_WIDTH,
  OLED_HEIGHT,
  &Wire,
  -1
);


// ============================================================
// STATE
// ============================================================

bool alertActive = false;

int activeWaypoint = -1;

unsigned long lastDisplayUpdate = 0;
unsigned long lastGPSStatusPrint = 0;


// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

bool gpsDataUsable();

bool highwayModeActive();

void initializeOLED();

void updateDisplay();

void checkCoordinates();

void printGPSInfo();


// ============================================================
// OLED INITIALIZATION
// ============================================================

void initializeOLED() {

  Wire.begin(
    runtimeSettings.screenSdaPin,
    runtimeSettings.screenSclPin
  );


  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {

    Serial.println(
      "ERROR: OLED initialization failed!"
    );

    while (true) {
      delay(1000);
    }
  }

  Serial.println(
    "OLED initialized."
  );
}


// ============================================================
// GPS VALIDITY
// ============================================================

bool gpsDataUsable() {

  if (!gps.location.isValid()) {
    return false;
  }


  if (runtimeSettings.minSatellites > 0) {

    if (!gps.satellites.isValid()) {
      return false;
    }

    if (
      gps.satellites.value() <
      runtimeSettings.minSatellites
    ) {

      return false;
    }
  }


  return true;
}


// ============================================================
// HIGHWAY MODE
// ============================================================

bool highwayModeActive() {

  return gps.speed.isValid() &&
    gps.speed.mph() >
      runtimeSettings.highwaySpeedMph;
}


// ============================================================
// DISTANCE TEXT
// ============================================================

// Writes a distance for the display, e.g. "350ft", "1.2mi",
// "107m", "3.4km". Switches from ft/m to mi/km above
// DISTANCE_SWITCH_FEET, using the units chosen in the config portal.
void formatDistance(
  double feet,
  char* out,
  size_t outSize
) {

  bool longDistance =
    feet > DISTANCE_SWITCH_FEET;

  if (runtimeSettings.metricUnits) {

    double meters =
      feet * 0.3048;

    if (longDistance) {

      double km =
        meters / 1000.0;

      snprintf(
        out,
        outSize,
        km < 100.0 ? "%.1fkm" : "%.0fkm",
        km
      );

    } else {

      snprintf(out, outSize, "%.0fm", meters);
    }

  } else {

    if (longDistance) {

      double miles =
        feet / 5280.0;

      snprintf(
        out,
        outSize,
        miles < 100.0 ? "%.1fmi" : "%.0fmi",
        miles
      );

    } else {

      snprintf(out, outSize, "%.0fft", feet);
    }
  }
}


// ============================================================
// SMARTER ALERTS
// ============================================================

static double angleDiff(double a, double b) {
  double d = fmod(fabs(a - b), 360.0);
  return d > 180.0 ? 360.0 - d : d;
}

// Once the car is moving fast enough for GPS heading to be reliable,
// skip cameras that are behind the car or watch a cross street.
bool cameraRelevant(int index, double distanceM) {

  if (!gps.course.isValid() ||
      !gps.speed.isValid() ||
      gps.speed.mph() < DIRECTION_FILTER_MIN_MPH) {
    return true;
  }

  double heading = gps.course.deg();
  const Camera& c = cameras[index];

  // Very close cameras skip the "ahead" test: bearing is noisy there.
  if (distanceM > 8.0) {
    double bearing = TinyGPSPlus::courseTo(
      gps.location.lat(), gps.location.lng(), c.lat, c.lon);
    if (angleDiff(bearing, heading) > AHEAD_CONE_DEGREES) {
      return false;
    }
  }

  if (c.direction >= 0) {
    double d = angleDiff(c.direction, heading);
    if (d > CROSS_STREET_DEGREES && d < 180.0 - CROSS_STREET_DEGREES) {
      return false;
    }
  }

  return true;
}


// Position of the last camera counted during the current alert,
// used to treat a cluster of cameras as one site.
float lastCountedLat = NAN;
float lastCountedLon = NAN;

void noteWaypointPassed(int index) {

  const Camera& c = cameras[index];

  if (!isnan(lastCountedLat) &&
      fastDistanceM(lastCountedLat, lastCountedLon, c.lat, c.lon) <
        CLUSTER_RADIUS_M) {
    return;
  }

  lastCountedLat = c.lat;
  lastCountedLon = c.lon;

  bool speedCam = isSpeedCamera(index);

  if (speedCam) {
    noteSpeedCamPassed();
  } else {
    noteAlprPassed();
  }

  // Speed cameras with a known limit only chime when over it.
  bool overLimit =
    c.limitMph == 0 ||
    !gps.speed.isValid() ||
    gps.speed.mph() > c.limitMph;

  if (!speedCam || overLimit) {
    startChime(speedCam);
  }
}


// ============================================================
// CHECK COORDINATES
// ============================================================

void checkCoordinates() {

  if (!gpsDataUsable()) {
    return;
  }


  double latitude =
    gps.location.lat();

  double longitude =
    gps.location.lng();


  double closestDistance;

  int closest =
    findClosestCamera(
      latitude,
      longitude,
      closestDistance,
      cameraRelevant
    );


  if (closest < 0) {
    return;
  }


  bool highwayMode =
    highwayModeActive();

  double alertDistanceFeet =
    highwayMode
    ? runtimeSettings.highwayAlertDistanceFeet
    : runtimeSettings.alertDistanceFeet;

  double alertDistanceMeters =
    alertDistanceFeet *
    0.3048;


  // Keep an alert active until its own detection area has been
  // left. This avoids an immediate clear when the highway radius
  // is larger than the normal clear distance.
  double activeAlertDistanceFeet =
    (highwayMode ||
     alertStartedInHighwayMode)
    ? runtimeSettings.highwayAlertDistanceFeet
    : runtimeSettings.alertDistanceFeet;

  double clearDistanceFeet =
    max(
      runtimeSettings.alertClearDistanceFeet,
      activeAlertDistanceFeet
    );

  double clearDistanceMeters =
    clearDistanceFeet *
    0.3048;


  // ==========================================================
  // ENTER ALERT
  // ==========================================================

  if (
    !alertActive &&
    closestDistance <=
      alertDistanceMeters
  ) {

    alertActive = true;

    activeWaypoint =
      closest;

    alertStartedInHighwayMode =
      highwayMode;


    bool speedCam =
      isSpeedCamera(closest);

    noteWaypointPassed(closest);


    Serial.println();
    Serial.println(
      "================================"
    );

    Serial.println(
      speedCam
      ? "       SPEED CAM DETECTED"
      : "         FLOCK DETECTED"
    );

    Serial.println(
      "================================"
    );


    Serial.print(
      "Location: "
    );

    Serial.println(
      cameraName(closest)
    );


    Serial.print(
      "Distance: "
    );

    Serial.print(
      closestDistance * 3.2808399,
      1
    );

    Serial.println(
      " ft"
    );


    Serial.print(
      "ALPRs passed: "
    );

    Serial.println(
      alprs_passed
    );

    Serial.print(
      "Speed cams passed: "
    );

    Serial.println(
      speedcams_passed
    );


    Serial.println(
      "================================"
    );
  }


  // ==========================================================
  // ACTIVE ALERT
  // ==========================================================

  if (alertActive) {

    if (
      closest != activeWaypoint &&
      closestDistance <=
        alertDistanceMeters
    ) {

      activeWaypoint =
        closest;

      noteWaypointPassed(closest);
    }


    double activeDistance =
      fastDistanceM(
        latitude,
        longitude,
        cameras[activeWaypoint].lat,
        cameras[activeWaypoint].lon
      );

    if (
      activeDistance >
      clearDistanceMeters
    ) {

      alertActive = false;

      lastCountedLat = NAN;

      activeWaypoint = -1;

      alertStartedInHighwayMode = false;

      Serial.println(
        "Left alert area."
      );
    }
  }
}


// ============================================================
// OLED DISPLAY
// ============================================================

void updateDisplay() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  // ==========================================================
  // CONFIGURATION WI-FI SCREEN
  // ==========================================================

  if (configPortalActive) {

    display.setTextSize(1);


    display.setCursor(
      0,
      0
    );

    display.println(
      "Wifi enabled"
    );


    display.setCursor(
      0,
      16
    );

    display.print(
      "SSID: "
    );

    display.println(
      CONFIG_AP_SSID
    );


    display.setCursor(
      0,
      30
    );

    display.print(
      "Password: "
    );

    display.println(
      CONFIG_AP_PASSWORD
    );


    display.setCursor(
      0,
      45
    );

    display.print(
      "192.168.4.1"
    );


    display.setCursor(
      0,
      57
    );

    display.print(
      "Configure in browser"
    );


    display.display();

    return;
  }


  // ==========================================================
  // NO GPS
  // ==========================================================

  if (!gpsDataUsable()) {

    display.setTextSize(2);

    display.setCursor(
      0,
      0
    );

    display.println(
      "GPS"
    );


    display.setTextSize(1);

    display.setCursor(
      0,
      24
    );

    display.println(
      "Waiting for fix..."
    );


    if (
      gps.satellites.isValid()
    ) {

      display.setCursor(
        0,
        40
      );

      display.print(
        "Satellites: "
      );

      display.println(
        gps.satellites.value()
      );
    }


    display.display();

    return;
  }


  // ==========================================================
  // GPS DATA
  // ==========================================================

  double latitude =
    gps.location.lat();

  double longitude =
    gps.location.lng();

  double speedMPH =
    gps.speed.isValid()
    ? gps.speed.mph()
    : 0.0;

  double speedShown =
    runtimeSettings.metricUnits
    ? speedMPH * 1.609344
    : speedMPH;

  const char* speedUnit =
    runtimeSettings.metricUnits ? "KMH" : "MPH";

  bool highwayMode =
    highwayModeActive();


  // ==========================================================
  // FIND CLOSEST
  // ==========================================================

  double closestDistance;

  int closest =
    findClosestCamera(
      latitude,
      longitude,
      closestDistance,
      cameraRelevant
    );


  double closestFeet =
    closestDistance *
    3.2808399;

  // While alerting, show the distance to the active camera.
  if (alertActive && activeWaypoint >= 0) {
    closestFeet =
      fastDistanceM(
        latitude,
        longitude,
        cameras[activeWaypoint].lat,
        cameras[activeWaypoint].lon
      ) * 3.2808399;
  }


  // ==========================================================
  // NEAR ALERT
  // ==========================================================

  if (
    runtimeSettings.autoScreenSwitch &&
    alertActive &&
    activeWaypoint >= 0
  ) {

    const char* alertText =
      "NEAR";


    display.setTextSize(4);


    int16_t x1;
    int16_t y1;

    uint16_t w;
    uint16_t h;


    display.getTextBounds(
      alertText,
      0,
      0,
      &x1,
      &y1,
      &w,
      &h
    );


    display.setCursor(
      (OLED_WIDTH - w) / 2,
      0
    );

    display.print(
      alertText
    );


    // --------------------------------------------------------
    // ALPR NAME
    // --------------------------------------------------------

    display.setTextSize(1);


    const char* name =
      cameraName(activeWaypoint);


    const int maxNameChars = 21;

    char displayName[
      maxNameChars + 1
    ];


    strncpy(
      displayName,
      name,
      maxNameChars
    );

    displayName[
      maxNameChars
    ] = '\0';


    int nameWidth =
      strlen(displayName) * 6;


    int nameX =
      (OLED_WIDTH -
       nameWidth) / 2;


    if (nameX < 0) {
      nameX = 0;
    }


    display.setCursor(
      nameX,
      34
    );

    display.print(
      displayName
    );


    // --------------------------------------------------------
    // SPEED
    // --------------------------------------------------------

    char speedText[20];


    snprintf(
      speedText,
      sizeof(speedText),
      "%.1f %s",
      speedShown,
      speedUnit
    );


    display.setTextSize(2);


    display.getTextBounds(
      speedText,
      0,
      0,
      &x1,
      &y1,
      &w,
      &h
    );


    display.setCursor(
      (OLED_WIDTH - w) / 2,
      48
    );

    display.print(
      speedText
    );


    display.display();

    return;
  }


  // ==========================================================
  // NORMAL SCREEN
  // ==========================================================

  char speedText[16];


  snprintf(
    speedText,
    sizeof(speedText),
    "%.1f",
    speedShown
  );


  display.setTextSize(
    runtimeSettings.speedTextSize
  );


  int16_t x1;
  int16_t y1;

  uint16_t w;
  uint16_t h;


  display.getTextBounds(
    speedText,
    0,
    0,
    &x1,
    &y1,
    &w,
    &h
  );


  int speedX =
    (OLED_WIDTH - w) / 2;


  display.setCursor(
    speedX,
    0
  );

  display.print(
    speedText
  );


  // MPH

  display.setTextSize(1);

  display.setCursor(
    46,
    31
  );

  display.print(
    speedUnit
  );


  // Nearest

  display.setCursor(
    0,
    41
  );

  display.print(
    "Nearest: "
  );


  char distanceText[16];

  formatDistance(
    closestFeet,
    distanceText,
    sizeof(distanceText)
  );


  if (closest >= 0) {

    display.print(
      distanceText
    );

    if (highwayMode) {

      display.print(
        " HWY"
      );
    }

  } else {

    display.print(
      "--"
    );
  }


  // Bottom line: alert bar while an ALPR or speed camera is active
  // (switching off), otherwise the coordinates.

  if (alertActive && activeWaypoint >= 0) {

    // Inverted bar: white background, black text.
    display.fillRect(
      0,
      53,
      OLED_WIDTH,
      11,
      SSD1306_WHITE
    );

    display.setTextColor(
      SSD1306_BLACK
    );

    display.setCursor(
      2,
      54
    );

    uint8_t limit =
      cameras[activeWaypoint].limitMph;

    if (isSpeedCamera(activeWaypoint) && limit > 0) {

      // e.g. "CAM 25MPH 350ft"
      display.print("CAM ");
      display.print(
        runtimeSettings.metricUnits
        ? (int)lround(limit * 1.609344)
        : (int)limit
      );
      display.print(
        runtimeSettings.metricUnits ? "KMH " : "MPH "
      );

    } else {

      display.print(
        isSpeedCamera(activeWaypoint)
        ? "SPEED CAM "
        : "ALPR NEAR "
      );
    }

    display.print(
      distanceText
    );

    display.setTextColor(
      SSD1306_WHITE
    );

  } else {

    display.setCursor(
      0,
      53
    );

    display.print(
      latitude,
      runtimeSettings.coordinateDecimals
    );

    display.print(
      ","
    );

    display.print(
      longitude,
      runtimeSettings.coordinateDecimals
    );
  }


  display.display();
}


// ============================================================
// SERIAL STATUS
// ============================================================

void printGPSInfo() {

  if (
    millis() -
    lastGPSStatusPrint <
    1000
  ) {

    return;
  }


  lastGPSStatusPrint =
    millis();


  Serial.print(
    "GPS: "
  );


  if (
    gps.location.isValid()
  ) {

    Serial.print(
      gps.location.lat(),
      6
    );

    Serial.print(
      ", "
    );

    Serial.print(
      gps.location.lng(),
      6
    );

  } else {

    Serial.print(
      "No fix"
    );
  }


  Serial.print(
    " | Speed: "
  );


  if (
    gps.speed.isValid()
  ) {

    Serial.print(
      gps.speed.mph(),
      1
    );

    Serial.print(
      " MPH"
    );

  } else {

    Serial.print(
      "--"
    );
  }


  Serial.print(
    " | Satellites: "
  );


  if (
    gps.satellites.isValid()
  ) {

    Serial.print(
      gps.satellites.value()
    );

  } else {

    Serial.print(
      "--"
    );
  }


  Serial.print(
    " | ALPRs passed: "
  );


  Serial.print(
    alprs_passed
  );

  Serial.print(
    " | Speed cams passed: "
  );

  Serial.println(
    speedcams_passed
  );
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(
    DEBUG_BAUD
  );


  delay(500);


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "       GPS ALERT SYSTEM"
  );

  Serial.println(
    "================================"
  );


  // ----------------------------------------------------------
  // BOOT BUTTON
  // ----------------------------------------------------------

  pinMode(
    BOOT_BUTTON_PIN,
    INPUT_PULLUP
  );


  // ----------------------------------------------------------
  // LOAD SAVED SETTINGS
  // ----------------------------------------------------------

  loadPersistentConfig();

  loadCameraDatabase();


  // ----------------------------------------------------------
  // GPS
  // ----------------------------------------------------------

  GPS_Serial.begin(
    runtimeSettings.gpsBaud,
    SERIAL_8N1,
    runtimeSettings.gpsRxPin,
    runtimeSettings.gpsTxPin
  );


  Serial.print(
    "GPS RX pin: GPIO"
  );

  Serial.println(
    runtimeSettings.gpsRxPin
  );


  Serial.print(
    "GPS TX pin: "
  );


  if (
    runtimeSettings.gpsTxPin >= 0
  ) {

    Serial.print(
      "GPIO"
    );

    Serial.println(
      runtimeSettings.gpsTxPin
    );

  } else {

    Serial.println(
      "disabled"
    );
  }


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  initializeOLED();


  // ----------------------------------------------------------
  // STARTUP SCREEN
  // ----------------------------------------------------------

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  display.setTextSize(2);

  display.setCursor(
    0,
    0
  );

  display.println(
    "GPS"
  );


  display.setTextSize(1);

  display.setCursor(
    0,
    24
  );

  display.println(
    "Coordinate Alert"
  );


  display.setCursor(
    0,
    36
  );

  display.print(
    "Points: "
  );

  display.println(
    cameraCount
  );


  display.setCursor(
    0,
    46
  );

  display.print(
    "ALPRs passed: "
  );

  display.println(
    alprs_passed
  );


  display.setCursor(
    0,
    56
  );

  display.print(
    "Speed cams passed: "
  );

  display.println(
    speedcams_passed
  );


  display.display();


  delay(1500);
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // BOOT HOLD / CONFIG MODE
  // ----------------------------------------------------------

  checkBootButton();


  // ----------------------------------------------------------
  // WEB SERVER
  // ----------------------------------------------------------

  handleConfigPortal();


  // ----------------------------------------------------------
  // GPS
  // ----------------------------------------------------------

  while (
    GPS_Serial.available()
  ) {

    gps.encode(
      GPS_Serial.read()
    );
  }


  // ----------------------------------------------------------
  // POSITION
  // ----------------------------------------------------------

  if (
    gps.location.isUpdated()
  ) {

    checkCoordinates();
  }


  // ----------------------------------------------------------
  // SERIAL
  // ----------------------------------------------------------

  printGPSInfo();


  // ----------------------------------------------------------
  // BUZZER CHIME
  // ----------------------------------------------------------

  updateBuzzer();


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  if (
    millis() -
    lastDisplayUpdate >=
    runtimeSettings.displayUpdateMs
  ) {

    lastDisplayUpdate =
      millis();

    updateDisplay();
  }
}
