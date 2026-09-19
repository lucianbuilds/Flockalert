#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// GPS
// ============================================================

constexpr int GPS_RX_PIN = 23;
constexpr int GPS_TX_PIN = -1;

constexpr uint32_t DEFAULT_GPS_BAUD = 9600;


// ============================================================
// OLED
// ============================================================

constexpr int OLED_SDA_PIN = 21;
constexpr int OLED_SCL_PIN = 22;

constexpr uint8_t OLED_ADDRESS = 0x3C;

constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;


// ============================================================
// FUTURE SPEAKER
// ============================================================

// Reserved for the future audio upgrade.
// Not currently used.

constexpr int SPEAKER_PIN = 25;


// ============================================================
// BOOT BUTTON / CONFIGURATION MODE
// ============================================================

// Standard ESP32 DevKit BOOT button.
constexpr int BOOT_BUTTON_PIN = 0;

// Button debounce.
constexpr unsigned long BOOT_DEBOUNCE_MS = 50;

// Three presses must occur within this amount of time.
constexpr unsigned long CONFIG_TRIPLE_PRESS_WINDOW_MS = 10000;


// ============================================================
// CONFIGURATION WI-FI
// ============================================================

constexpr const char* CONFIG_AP_SSID =
  "Config-alpr-detector";

constexpr const char* CONFIG_AP_PASSWORD =
  "12345678";

// Wi-Fi channel used by the configuration AP.
constexpr uint8_t CONFIG_AP_CHANNEL = 1;

// Maximum devices connected to the configuration AP.
constexpr uint8_t CONFIG_AP_MAX_CONNECTIONS = 2;


// ============================================================
// DEFAULT PROXIMITY SETTINGS
// ============================================================

// Alert when within 50 feet.
constexpr double DEFAULT_ALERT_DISTANCE_FEET = 50.0;

// Alert clears after getting farther than 75 feet.
// This prevents GPS jitter around the 50-foot boundary.
constexpr double DEFAULT_ALERT_CLEAR_DISTANCE_FEET = 75.0;


// ============================================================
// DEFAULT DISPLAY SETTINGS
// ============================================================

constexpr uint32_t DEFAULT_DISPLAY_UPDATE_MS = 250;

constexpr int DEFAULT_COORDINATE_DECIMALS = 5;

// 4 = very large speed display.
constexpr int DEFAULT_SPEED_TEXT_SIZE = 4;


// ============================================================
// DEFAULT GPS QUALITY SETTINGS
// ============================================================

// 0 = no minimum satellite requirement.
constexpr int DEFAULT_MIN_SATELLITES = 0;


// ============================================================
// SERIAL DEBUG
// ============================================================

constexpr unsigned long DEBUG_BAUD = 115200;

#endif