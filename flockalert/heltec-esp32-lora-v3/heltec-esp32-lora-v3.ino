#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>

#include "config.h"
#include "coordinates.h"
#include "configportal.h"


// ============================================================
// GLOBALS USED BY CONFIGPORTAL.H
// ============================================================

RuntimeSettings runtimeSettings;

uint32_t alprs_passed = 0;

bool configPortalActive = false;

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

int findClosestWaypoint(
  double latitude,
  double longitude,
  double &closestDistance
);

void initializeOLED();

void updateDisplay();

void checkCoordinates();

void printGPSInfo();


// ============================================================
// OLED INITIALIZATION
// ============================================================

void initializeOLED() {

  // ----------------------------------------------------------
  // TURN ON VEXT
  // ----------------------------------------------------------
  //
  // Heltec V3 uses GPIO36 for Vext control.
  // LOW = VEXT ON.
  //

  pinMode(
    VEXT_PIN,
    OUTPUT
  );

  digitalWrite(
    VEXT_PIN,
    LOW
  );

  // Give the OLED power rail time to come up.
  delay(100);


  // ----------------------------------------------------------
  // OLED RESET
  // ----------------------------------------------------------

  pinMode(
    OLED_RESET_PIN,
    OUTPUT
  );

  digitalWrite(
    OLED_RESET_PIN,
    LOW
  );

  delay(20);

  digitalWrite(
    OLED_RESET_PIN,
    HIGH
  );

  delay(20);


  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin(
    OLED_SDA_PIN,
    OLED_SCL_PIN
  );


  // ----------------------------------------------------------
  // SSD1306
  // ----------------------------------------------------------

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
    "Heltec OLED initialized."
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

    if (gps.satellites.value() <
        runtimeSettings.minSatellites) {

      return false;
    }
  }

  return true;
}


// ============================================================
// FIND CLOSEST COORDINATE
// ============================================================

int findClosestWaypoint(
  double latitude,
  double longitude,
  double &closestDistance
) {

  closestDistance = 1e30;

  int closestIndex = -1;

  for (
    size_t i = 0;
    i < WAYPOINT_COUNT;
    i++
  ) {

    double distance =
      TinyGPSPlus::distanceBetween(
        latitude,
        longitude,
        waypoints[i].latitude,
        waypoints[i].longitude
      );

    if (distance < closestDistance) {

      closestDistance = distance;
      closestIndex = i;
    }
  }

  return closestIndex;
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
    findClosestWaypoint(
      latitude,
      longitude,
      closestDistance
    );

  if (closest < 0) {
    return;
  }


  // ----------------------------------------------------------
  // CONVERT CONFIGURED FEET TO METERS
  // ----------------------------------------------------------

  const double alertDistanceMeters =
    runtimeSettings.alertDistanceFeet *
    0.3048;

  const double clearDistanceMeters =
    runtimeSettings.alertClearDistanceFeet *
    0.3048;


  // ==========================================================
  // ENTER ALERT ZONE
  // ==========================================================

  if (
    !alertActive &&
    closestDistance <= alertDistanceMeters
  ) {

    alertActive = true;

    activeWaypoint = closest;


    // Count one ALPR pass.
    noteAlprPassed();


    Serial.println();
    Serial.println(
      "================================"
    );

    Serial.println(
      "         FLOCK DETECTED"
    );

    Serial.println(
      "================================"
    );


    Serial.print(
      "Location: "
    );

    Serial.println(
      waypoints[closest].name
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
      "Speed: "
    );

    if (gps.speed.isValid()) {

      Serial.print(
        gps.speed.mph(),
        1
      );

    } else {

      Serial.print(
        "--"
      );
    }

    Serial.println(
      " MPH"
    );


    Serial.print(
      "ALPRs passed: "
    );

    Serial.println(
      alprs_passed
    );


    Serial.println(
      "================================"
    );
  }


  // ==========================================================
  // ACTIVE ALERT
  // ==========================================================

  if (alertActive) {


    // --------------------------------------------------------
    // ANOTHER COORDINATE BECOMES NEAREST
    // --------------------------------------------------------

    if (
      closest != activeWaypoint &&
      closestDistance <= alertDistanceMeters
    ) {

      activeWaypoint = closest;

      noteAlprPassed();


      Serial.println();

      Serial.println(
        "Another ALPR zone entered."
      );

      Serial.print(
        "Location: "
      );

      Serial.println(
        waypoints[closest].name
      );
    }


    // --------------------------------------------------------
    // LEAVE ALERT ZONE
    // --------------------------------------------------------

    if (
      closestDistance >
      clearDistanceMeters
    ) {

      alertActive = false;

      activeWaypoint = -1;


      Serial.println(
        "Left alert area."
      );
    }
  }
}


// ============================================================
// OLED UPDATE
// ============================================================

void updateDisplay() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  // ==========================================================
  // WAITING FOR GPS
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


    if (gps.satellites.isValid()) {

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
  // CURRENT GPS DATA
  // ==========================================================

  double latitude =
    gps.location.lat();

  double longitude =
    gps.location.lng();

  double speedMPH =
    gps.speed.isValid()
    ? gps.speed.mph()
    : 0.0;


  // ==========================================================
  // FIND NEAREST ALPR
  // ==========================================================

  double closestDistance;

  int closest =
    findClosestWaypoint(
      latitude,
      longitude,
      closestDistance
    );

  double closestFeet =
    closestDistance * 3.2808399;


  // ==========================================================
  // FULL-SCREEN NEAR ALERT
  // ==========================================================

  if (
    alertActive &&
    activeWaypoint >= 0
  ) {

    // --------------------------------------------------------
    // LARGE "NEAR"
    // --------------------------------------------------------

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
      waypoints[activeWaypoint].name;


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
      (OLED_WIDTH - nameWidth) / 2;


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
      "%.1f MPH",
      speedMPH
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
  // NORMAL DISPLAY
  // ==========================================================

  // ----------------------------------------------------------
  // LARGE SPEED
  // ----------------------------------------------------------

  char speedText[16];

  snprintf(
    speedText,
    sizeof(speedText),
    "%.1f",
    speedMPH
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


  // ----------------------------------------------------------
  // MPH
  // ----------------------------------------------------------

  display.setTextSize(1);

  display.setCursor(
    46,
    31
  );

  display.print(
    "MPH"
  );


  // ----------------------------------------------------------
  // NEAREST ALPR DISTANCE
  // ----------------------------------------------------------

  display.setCursor(
    0,
    41
  );

  display.print(
    "Nearest: "
  );


  if (closest >= 0) {

    display.print(
      closestFeet,
      0
    );

    display.print(
      "ft"
    );

  } else {

    display.print(
      "--"
    );
  }


  // ----------------------------------------------------------
  // COMPACT COORDINATES
  // ----------------------------------------------------------

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


  display.display();
}


// ============================================================
// SERIAL GPS DEBUG
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


  if (gps.location.isValid()) {

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


  if (gps.speed.isValid()) {

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


  if (gps.satellites.isValid()) {

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


  Serial.println(
    alprs_passed
  );
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  // ----------------------------------------------------------
  // SERIAL
  // ----------------------------------------------------------

  Serial.begin(
    DEBUG_BAUD
  );

  delay(500);


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "   HELTEC GPS ALERT SYSTEM"
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


  Serial.print(
    "GPS baud: "
  );

  Serial.println(
    runtimeSettings.gpsBaud
  );


  Serial.print(
    "Alert distance: "
  );

  Serial.print(
    runtimeSettings.alertDistanceFeet,
    1
  );

  Serial.println(
    " ft"
  );


  Serial.print(
    "Clear distance: "
  );

  Serial.print(
    runtimeSettings.alertClearDistanceFeet,
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


  // ----------------------------------------------------------
  // GPS UART
  // ----------------------------------------------------------

  GPS_Serial.begin(
    runtimeSettings.gpsBaud,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );


  Serial.print(
    "GPS RX: GPIO"
  );

  Serial.println(
    GPS_RX_PIN
  );


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  initializeOLED();


  // ----------------------------------------------------------
  // STARTUP DISPLAY
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
    40
  );

  display.print(
    "Points: "
  );

  display.println(
    WAYPOINT_COUNT
  );


  display.setCursor(
    0,
    52
  );

  display.print(
    "Passed: "
  );

  display.println(
    alprs_passed
  );


  display.display();


  delay(1500);


  // ----------------------------------------------------------
  // STATUS
  // ----------------------------------------------------------

  Serial.println();

  Serial.print(
    "Monitoring "
  );

  Serial.print(
    WAYPOINT_COUNT
  );

  Serial.println(
    " coordinates."
  );


  Serial.println(
    "Waiting for GPS fix..."
  );
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // BOOT BUTTON / CONFIG MODE
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
  // POSITION CHECK
  // ----------------------------------------------------------

  if (
    gps.location.isUpdated()
  ) {

    checkCoordinates();
  }


  // ----------------------------------------------------------
  // DEBUG
  // ----------------------------------------------------------

  printGPSInfo();


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