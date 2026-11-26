#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// DEFAULT GPS PINS
// ============================================================

constexpr int DEFAULT_GPS_RX_PIN = 23;
constexpr int DEFAULT_GPS_TX_PIN = -1;   // -1 = unused

constexpr uint32_t DEFAULT_GPS_BAUD = 9600;


// ============================================================
// DEFAULT OLED PINS
// ============================================================

constexpr int DEFAULT_SCREEN_SDA_PIN = 21;
constexpr int DEFAULT_SCREEN_SCL_PIN = 22;

constexpr uint8_t OLED_ADDRESS = 0x3C;

constexpr int OLED_WIDTH = 128;
constexpr int OLED_HEIGHT = 64;


// ============================================================
// DEFAULT SPEAKER PIN
// ============================================================
// Reserved for the future speaker/audio upgrade.

constexpr int DEFAULT_SPEAKER_PIN = 25;


// ============================================================
// BOOT BUTTON
// ============================================================

// Standard ESP32 DevKit BOOT button.
constexpr int BOOT_BUTTON_PIN = 0;

// Hold BOOT this long to enter configuration mode.
constexpr unsigned long BOOT_HOLD_TIME_MS = 3000;

constexpr unsigned long BOOT_DEBOUNCE_MS = 50;


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
// DEFAULT NORMAL ALERT SETTINGS
// ============================================================

constexpr double DEFAULT_ALERT_DISTANCE_FEET = 50.0;

constexpr double DEFAULT_ALERT_CLEAR_DISTANCE_FEET = 75.0;


// ============================================================
// HIGHWAY MODE
// ============================================================

// Highway mode activates when speed is ABOVE this value.
constexpr double DEFAULT_HIGHWAY_SPEED_MPH = 45.0;

// Alert distance while highway mode is active.
constexpr double DEFAULT_HIGHWAY_ALERT_DISTANCE_FEET = 150.0;


// ============================================================
// DEFAULT DISPLAY SETTINGS
// ============================================================

constexpr uint32_t DEFAULT_DISPLAY_UPDATE_MS = 250;

constexpr int DEFAULT_COORDINATE_DECIMALS = 5;

// 4 = very large speed display.
constexpr int DEFAULT_SPEED_TEXT_SIZE = 4;

// true = switch to the full-screen NEAR alert screen when an ALPR is close.
// false = stay on the speed screen and show a small alert bar instead.
constexpr bool DEFAULT_AUTO_SCREEN_SWITCH = false;


// ============================================================
// BUZZER
// ============================================================

constexpr bool DEFAULT_BUZZER_ENABLED = false;

// Index into CHIME_TONE_NAMES below.
constexpr int DEFAULT_CHIME_TONE = 0;

constexpr int CHIME_TONE_COUNT = 4;

// Names shown on the configuration page. Sequences live in buzzer.h.
constexpr const char* CHIME_TONE_NAMES[CHIME_TONE_COUNT] = {
  "Two-tone",
  "Rising",
  "Warble",
  "Quick beep"
};


// ============================================================
// GPS QUALITY
// ============================================================

// 0 = no minimum satellite requirement.
constexpr int DEFAULT_MIN_SATELLITES = 0;


// ============================================================
// SERIAL DEBUG
// ============================================================

constexpr unsigned long DEBUG_BAUD = 115200;

#endif