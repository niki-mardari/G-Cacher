#pragma once

#include <Arduino.h>

// ------------------------------------------------------------
// User settings
// ------------------------------------------------------------
// Keep private details out of the source file.
// Fill these in locally before flashing, or later replace this with SD/NVS settings.

const char *const WIFI_SSID = "yep";        // Put your Wi-Fi SSID here
const char *const WIFI_PASSWORD = "12345678";    // Put your Wi-Fi password here

const char *const SERVER_HOST = "172.26.139.42";      // Example: "192.168.1.50" or "my-server.local"
const char *const TELEMETRY_URL = "http://172.26.139.42:8000/api/telemetry";    // Example: "http://192.168.1.50:8000/api/telemetry"
const char *const ASSETS_URL = "http://172.26.139.42:8000/api/assets";       // Example: "http://192.168.1.50:8000/api/assets"

const char *const DEVICE_ID = "tenstar_001";

// ------------------------------------------------------------
// I2C sensors
// ------------------------------------------------------------

constexpr int I2C_SDA = 42;
constexpr int I2C_SCL = 41;

constexpr uint8_t BMP_ADDR_1 = 0x77;
constexpr uint8_t BMP_ADDR_2 = 0x76;

constexpr uint8_t QMI_ADDR_1 = 0x6B;
constexpr uint8_t QMI_ADDR_2 = 0x6A;

// ------------------------------------------------------------
// LC29H GNSS UART
// ------------------------------------------------------------
// Current sketch mapping kept from your uploaded firmware:
// LC29H TX -> ESP32 GPIO17, LC29H RX -> ESP32 GPIO18.

constexpr int GNSS_RX_PIN = 17;
constexpr int GNSS_TX_PIN = 18;
constexpr uint32_t GNSS_BAUD = 115200;

// ------------------------------------------------------------
// TFT + touch SPI
// ------------------------------------------------------------

constexpr int BIG_TFT_CS = 8;
constexpr int BIG_TFT_RST = 14;
constexpr int BIG_TFT_DC = 15;
constexpr int BIG_TFT_MOSI = 11;
constexpr int BIG_TFT_SCLK = 12;
constexpr int BIG_TFT_MISO = 13;

constexpr int TOUCH_CS = 10;
constexpr int TOUCH_IRQ = -1;

// ------------------------------------------------------------
// Dedicated SD SPI
// ------------------------------------------------------------

constexpr int SD_CS = 9;
constexpr int SD_SCLK = 36;
constexpr int SD_MOSI = 35;
constexpr int SD_MISO = 37;

constexpr uint32_t SD_FREQ_SLOW = 400000;
constexpr uint32_t SD_FREQ_MED = 1000000;
constexpr uint32_t SD_FREQ_FAST = 4000000;

const char *const LOG_FILE = "/sat_cacher_log.csv";
constexpr uint32_t LOG_INTERVAL_MS = 5000;

// ------------------------------------------------------------
// Buzzer
// ------------------------------------------------------------

constexpr int BUZZER_PIN = 5;
constexpr int BUZZER_CLICK_FREQ = 1300;
constexpr uint16_t BUZZER_CLICK_MS = 18;

// ------------------------------------------------------------
// Display and touch
// ------------------------------------------------------------

constexpr int BIG_W = 320;
constexpr int BIG_H = 480;
constexpr int DISPLAY_ROTATION = 2;

// UI refresh checks are cheap because fields only redraw when values change.
constexpr uint32_t DISPLAY_UPDATE_MS = 750;

constexpr int NAV_Y = 410;
constexpr int NAV_H = 70;
constexpr int NAV_BACK_X0 = 0;
constexpr int NAV_BACK_X1 = 106;
constexpr int NAV_HOME_X0 = 107;
constexpr int NAV_HOME_X1 = 213;
constexpr int NAV_NEXT_X0 = 214;
constexpr int NAV_NEXT_X1 = 319;

constexpr int SAVE_BTN_X = 18;
constexpr int SAVE_BTN_Y = 80;
constexpr int SAVE_BTN_W = 284;
constexpr int SAVE_BTN_H = 46;

constexpr int BOOT_BUTTON_PIN = 0;
constexpr bool TOUCH_INVERT_X = false;

// ------------------------------------------------------------
// Network timing
// ------------------------------------------------------------
// These values are intentionally slower than the original sketch so a bad Wi-Fi
// password or offline server does not constantly interrupt the touch UI.

constexpr uint32_t WIFI_RETRY_START_MS = 60000;    // 1 minute
constexpr uint32_t WIFI_RETRY_MAX_MS = 300000;     // 5 minutes

constexpr uint32_t UPLOAD_INTERVAL_MS = 15000;     // normal upload interval when healthy
constexpr uint32_t SERVER_RETRY_START_MS = 30000;  // 30 seconds after a server failure
constexpr uint32_t SERVER_RETRY_MAX_MS = 300000;   // 5 minutes

constexpr uint16_t HTTP_CONNECT_TIMEOUT_MS = 500;
constexpr uint16_t HTTP_READ_TIMEOUT_MS = 800;
