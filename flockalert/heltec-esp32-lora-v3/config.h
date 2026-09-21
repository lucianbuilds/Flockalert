#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// HELTEC WIFI LORA 32 V3
// ESP32-S3
// ============================================================


// ============================================================
// GPS
// ============================================================

// GPS TX -> Heltec GPIO2
//
// GPIO2 is a general-purpose GPIO and is not used by the
// onboard OLED or LoRa radio.

constexpr int GPS_RX_PIN = 2;

// GPS transmit pin.
// We are only receiving NMEA data from the GPS, so no TX pin
// is required.

constexpr int GPS_TX_PIN = -1;

// Default u-blox baud rate.
constexpr uint32_t DEFAULT_GPS_BAUD = 9600;


// ============================================================
// ONBOARD OLED
// ============================================================

constexpr int OLED_SDA_PIN = 17;
constexpr int OLED_SCL_PIN = 18;

// Heltec V3 onboard OLED reset.
constexpr int OLED_RESET_PIN = 21;

// Vext controls power to the OLED.
// On the V3, LOW turns Vext on.
constexpr int VEXT_PIN = 36;

constexpr uint8_t OLED_ADDRESS = 0x3C;

constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;


// ============================================================
// BOOT / PRG BUTTON
// ============================================================

// Heltec PRG/BOOT button.
constexpr int BOOT_BUTTON_PIN = 0;

// Button debounce.
constexpr unsigned long BOOT_DEBOUNCE_MS = 50;

// Three presses must occur within 2 seconds.
constexpr unsigned long CONFIG_TRIPLE_PRESS_WINDOW_MS = 2000;


// ============================================================
// CONFIGURATION WI-FI
// ============================================================

constexpr const char* CONFIG_AP_SSID =
  "GPS-ALPR-Config";

constexpr const char* CONFIG_AP_PASSWORD =
  "gpsconfig";

constexpr uint8_t CONFIG_AP_CHANNEL = 1;

constexpr uint8_t CONFIG_AP_MAX_CONNECTIONS = 2;


// ============================================================
// PROXIMITY ALERT
// ============================================================

constexpr double DEFAULT_ALERT_DISTANCE_FEET = 50.0;

constexpr double DEFAULT_ALERT_CLEAR_DISTANCE_FEET = 75.0;


// ============================================================
// DISPLAY
// ============================================================

constexpr uint32_t DEFAULT_DISPLAY_UPDATE_MS = 250;

constexpr int DEFAULT_COORDINATE_DECIMALS = 5;

// 4 = very large speed display.
constexpr int DEFAULT_SPEED_TEXT_SIZE = 4;


// ============================================================
// GPS QUALITY
// ============================================================

// 0 = no minimum satellite requirement.

constexpr int DEFAULT_MIN_SATELLITES = 0;


// ============================================================
// SERIAL DEBUG
// ============================================================

constexpr unsigned long DEBUG_BAUD = 115200;


// ============================================================
// FUTURE AUDIO
// ============================================================

// Reserved for the future speaker/audio upgrade.

constexpr int SPEAKER_PIN = 25;

#endif