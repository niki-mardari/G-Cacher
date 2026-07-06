/********************************************************
  Sat Cacher Rev2 Clean UI
  Tenstar ESP32-S3 + LC29H GNSS + ILI9488/XPT2046 + SD + Backend

  Last updated: 2026-07-01

  What this firmware does:
  - Reads live GNSS data from the LC29H using TinyGPSPlus
  - Shows a simple touch menu called Sat Cacher
  - Shows Live GNSS, Signal Quality, FSPL, System Status, and Geocache screens
  - Logs GNSS/sensor/server data to the SD card
  - Uploads telemetry to the FastAPI backend
  - Receives the current geocache command/message from the backend
  - Lets the user save the current location as a marked spot using /api/assets

  Main anti-flicker design:
  - Full screen redraw only happens when changing menu
  - Live values update only inside small cleared rectangles
  - Display updates run every DISPLAY_UPDATE_MS, not continuously

  Required libraries:
  - LovyanGFX
  - TinyGPSPlus
  - Adafruit BMP280
  - ArduinoJson
  - ImuDrv.hpp for the QMI8658C
********************************************************/

#define LGFX_USE_V1

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <math.h>
#include <string.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <LovyanGFX.hpp>
#include <TinyGPSPlus.h>
#include <Adafruit_BMP280.h>

#include "ImuDrv.hpp"

// ------------------------------------------------------------
// User settings
// ------------------------------------------------------------

const char *WIFI_SSID = ""; // Put your Wi-Fi SSID here
const char *WIFI_PASSWORD = ""; // Put your Wi-Fi password here

//Server IP.
const char *SERVER_HOST = ""; // Replace with your FastAPI server IP
// Replace with "http://<server_ip>:8000/api/telemetry"
const char *TELEMETRY_URL = "";
// Replace with "http://<server_ip>:8000/api/assets"
const char *ASSETS_URL    = "";

// Device ID for telemetry and assets. Change this for each device.
const char *DEVICE_ID = "tenstar_001";

// ------------------------------------------------------------
// I2C sensors
// ------------------------------------------------------------

#define I2C_SDA 42
#define I2C_SCL 41

#define BMP_ADDR_1 0x77
#define BMP_ADDR_2 0x76

#define QMI_ADDR_1 0x6B
#define QMI_ADDR_2 0x6A

// ------------------------------------------------------------
// LC29H GNSS UART
// ------------------------------------------------------------

#define GNSS_RX_PIN 17
#define GNSS_TX_PIN 18
#define GNSS_BAUD   115200

// ------------------------------------------------------------
// TFT + touch SPI
// ------------------------------------------------------------

#define BIG_TFT_CS    8
#define BIG_TFT_RST   14
#define BIG_TFT_DC    15
#define BIG_TFT_MOSI  11
#define BIG_TFT_SCLK  12
#define BIG_TFT_MISO  13

#define TOUCH_CS      10
#define TOUCH_IRQ     -1

// ------------------------------------------------------------
// Dedicated SD SPI
// ------------------------------------------------------------

#define SD_CS    9
#define SD_SCLK  36
#define SD_MOSI  35
#define SD_MISO  37

#define SD_FREQ_SLOW 400000
#define SD_FREQ_MED  1000000
#define SD_FREQ_FAST 4000000

#define LOG_FILE "/sat_cacher_log.csv"
#define LOG_INTERVAL_MS 5000

// ------------------------------------------------------------
// Buzzer
// ------------------------------------------------------------

#define BUZZER_PIN 5
#define BUZZER_FREQ 1300
#define BUZZER_BEEP_MS 18

// ------------------------------------------------------------
// Display and touch
// ------------------------------------------------------------

#define BIG_W 320
#define BIG_H 480
#define DISPLAY_ROTATION 2

// Keep this slower to stop constant redrawing.
#define DISPLAY_UPDATE_MS 1200

// Web upload timing.
#define UPLOAD_INTERVAL_MS 5000
#define WIFI_RETRY_INTERVAL_MS 10000
#define HTTP_TIMEOUT_MS 2500

// Bottom nav bar.
#define NAV_Y 410
#define NAV_H 70
#define NAV_BACK_X0 0
#define NAV_BACK_X1 106
#define NAV_HOME_X0 107
#define NAV_HOME_X1 213
#define NAV_NEXT_X0 214
#define NAV_NEXT_X1 319

// Save button on Geocache screen.
#define SAVE_BTN_X 18
#define SAVE_BTN_Y 330
#define SAVE_BTN_W 284
#define SAVE_BTN_H 46

#define BOOT_BUTTON_PIN 0
#define TOUCH_INVERT_X false

// ------------------------------------------------------------
// LovyanGFX class
// ------------------------------------------------------------

class LGFX_ILI9488_Touch : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX_ILI9488_Touch(void)
  {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI3_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read = 6000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = BIG_TFT_SCLK;
      cfg.pin_mosi = BIG_TFT_MOSI;
      cfg.pin_miso = BIG_TFT_MISO;
      cfg.pin_dc   = BIG_TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();
      cfg.pin_cs   = BIG_TFT_CS;
      cfg.pin_rst  = BIG_TFT_RST;
      cfg.pin_busy = -1;
      cfg.panel_width  = 320;
      cfg.panel_height = 480;
      cfg.memory_width  = 320;
      cfg.memory_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = false;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel.config(cfg);
    }

    {
      auto cfg = _touch.config();
      cfg.spi_host = SPI3_HOST;
      cfg.freq = 1000000;
      cfg.pin_sclk = BIG_TFT_SCLK;
      cfg.pin_mosi = BIG_TFT_MOSI;
      cfg.pin_miso = BIG_TFT_MISO;
      cfg.pin_cs   = TOUCH_CS;
      cfg.pin_int  = TOUCH_IRQ;
      cfg.bus_shared = true;
      cfg.x_min = 200;
      cfg.x_max = 3900;
      cfg.y_min = 200;
      cfg.y_max = 3900;
      cfg.offset_rotation = 0;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};

// ------------------------------------------------------------
// Global objects
// ------------------------------------------------------------

LGFX_ILI9488_Touch tft;
SPIClass sdSPI(FSPI);

HardwareSerial GNSS(1);
TinyGPSPlus gps;

Adafruit_BMP280 bmp;
SensorQMI8658 qmi;
AccelerometerData acc;
GyroscopeData gyr;

// ------------------------------------------------------------
// State
// ------------------------------------------------------------

bool bmpOK = false;
bool qmiOK = false;
bool sdOK = false;
bool wifiOK = false;
bool serverOK = false;
bool touchOK = false;

char sdStatusText[20] = "NOT STARTED";
char wifiStatusText[20] = "OFFLINE";
char serverStatusText[24] = "NOT SENT";
char saveStatusText[42] = "No marked location saved yet";

float bmpTempC = 0.0f;
float bmpPressureHpa = 0.0f;
float accX = 0.0f;
float accY = 0.0f;
float accZ = 0.0f;
float accMagnitude = 0.0f;

uint32_t gnssChars = 0;
uint32_t lastGnssCharTime = 0;
char nmeaLine[150];
uint8_t nmeaIndex = 0;
char lastNmeaLine[150] = "No NMEA yet";

uint16_t rawTouchX = 0;
uint16_t rawTouchY = 0;
uint16_t touchX = 0;
uint16_t touchY = 0;
bool touchPressed = false;
bool touchWasDown = false;

unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastLogTime = 0;
unsigned long lastUploadTime = 0;
unsigned long lastWiFiRetry = 0;
unsigned long lastTouchAction = 0;
unsigned long lastBootPress = 0;
unsigned long lastSerialPrint = 0;

uint32_t logRows = 0;
uint32_t uploadCount = 0;
uint32_t uploadFailCount = 0;
int lastHttpCode = 0;

char serverCommand[32] = "WAITING";
char serverColour[20] = "yellow";
char serverMessage[150] = "Waiting for telemetry upload";
char serverTarget[64] = "No target yet";
float serverDistanceM = -1.0f;

int lastSavedAssetId = -1;

// ------------------------------------------------------------
// Menu system
// ------------------------------------------------------------

enum MenuPage
{
  MENU_HOME = 0,
  MENU_LIVE_GNSS,
  MENU_SIGNAL_QUALITY,
  MENU_FSPL,
  MENU_SYSTEM,
  MENU_GEOCACHE,
  MENU_COUNT
};

MenuPage currentMenu = MENU_HOME;
bool forceRedraw = true;

// ------------------------------------------------------------
// Prototypes
// ------------------------------------------------------------

void initPins();
void initDisplay();
void initSensors();
void initGNSS();
void initSDCard();
bool tryStartSD(uint32_t frequency);
void createLogHeaderIfNeeded();
void appendLogRow();
void printFloatOrBlank(File &file, bool valid, double value, int decimals);

void updateGNSS();
void updateSensors();
void updateTouch();
void updateSDLogging();
void updateWiFiNonBlocking();
void updateWebUpload();

void uploadTelemetry();
void parseServerReply(const String &response);
void saveMarkedLocation();
void safeCopy(char *dest, size_t destSize, const char *src);

void handleSerialInput();
void handleBootButton();

bool readMappedTouch(uint16_t *xOut, uint16_t *yOut);
void processTouch(uint16_t x, uint16_t y);

void drawScreen();
void updateScreenValues();
void drawHeader(const char *title);
void drawNavBar();
void drawBackIcon(int cx, int cy, uint16_t colour);
void drawHomeIcon(int cx, int cy, uint16_t colour);
void drawNextIcon(int cx, int cy, uint16_t colour);
void drawLabelValue(int x, int y, const char *label, const char *value, uint16_t valueColour = TFT_WHITE);
void clearValueArea(int x, int y, int w = 185, int h = 24);

void drawHomeMenu();
void updateHomeValues();
void drawLiveGNSSMenu();
void updateLiveGNSSValues();
void drawSignalQualityMenu();
void updateSignalQualityValues();
void drawFSPLMenu();
void drawSystemMenu();
void updateSystemValues();
void drawGeocacheMenu();
void updateGeocacheValues();

void nextMenu();
void previousMenu();
void goHome();

void beep();
void startupBeep();

bool gnssReceiving();
bool gnssHasFix();
int gnssSatellites();
float gnssHdop();
const char *qualityText();
const char *qualityReason();
uint16_t qualityColour();
uint16_t colourFromServer(const char *name);
const char *thermalStatus();
uint16_t thermalColour();
float calculateFSPL(float distanceKm, float frequencyMHz);
void printIPAddressToScreen(int x, int y);

// ------------------------------------------------------------
// Setup and loop
// ------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("====================================");
  Serial.println("Sat Cacher Rev2 Clean UI");
  Serial.println("====================================");
  Serial.println("LC29H TX -> GPIO17, LC29H RX -> GPIO18");
  Serial.println("SD: SCK36 MOSI35 MISO37 CS9");
  Serial.print("Telemetry URL: ");
  Serial.println(TELEMETRY_URL);

  initPins();
  startupBeep();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  initDisplay();
  initSensors();
  initGNSS();
  initSDCard();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  safeCopy(wifiStatusText, sizeof(wifiStatusText), "CONNECTING");
  lastWiFiRetry = millis();

  drawScreen();

  Serial.println("Setup complete");
  Serial.println("Controls: n=next, p=previous, h=home, 1-5=menu, u=upload, m=mark location");
}

void loop()
{
  handleSerialInput();
  handleBootButton();

  updateGNSS();
  updateTouch();
  updateSensors();
  updateWiFiNonBlocking();
  updateWebUpload();
  updateSDLogging();

  if (forceRedraw)
  {
    drawScreen();
    forceRedraw = false;
    lastDisplayUpdate = millis();
  }

  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_MS)
  {
    lastDisplayUpdate = millis();
    updateScreenValues();
  }
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------

void initPins()
{
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(BIG_TFT_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(BIG_TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
}

void initDisplay()
{
  bool ok = tft.init();
  Serial.print("Display init: ");
  Serial.println(ok ? "OK" : "FAILED");

  tft.setRotation(DISPLAY_ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(32, 210);
  tft.print("Sat Cacher");
  delay(500);
}

void initSensors()
{
  if (bmp.begin(BMP_ADDR_1))
  {
    bmpOK = true;
    Serial.println("BMP280 found at 0x77");
  }
  else if (bmp.begin(BMP_ADDR_2))
  {
    bmpOK = true;
    Serial.println("BMP280 found at 0x76");
  }
  else
  {
    bmpOK = false;
    Serial.println("BMP280 not found");
  }

  if (qmi.begin(Wire, QMI_ADDR_1, I2C_SDA, I2C_SCL))
  {
    qmiOK = true;
    Serial.println("QMI8658C found at 0x6B");
  }
  else if (qmi.begin(Wire, QMI_ADDR_2, I2C_SDA, I2C_SCL))
  {
    qmiOK = true;
    Serial.println("QMI8658C found at 0x6A");
  }
  else
  {
    qmiOK = false;
    Serial.println("QMI8658C not found");
    return;
  }

  qmi.configAccel(AccelFullScaleRange::FS_8G, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
  qmi.configGyro(GyroFullScaleRange::FS_1000_DPS, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
  qmi.enableAccel();
  qmi.enableGyro();
}

void initGNSS()
{
  GNSS.setRxBufferSize(2048);
  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  Serial.println("LC29H UART started");
}

void initSDCard()
{
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  sdOK = tryStartSD(SD_FREQ_SLOW);
  if (!sdOK) sdOK = tryStartSD(SD_FREQ_MED);
  if (!sdOK) sdOK = tryStartSD(SD_FREQ_FAST);

  if (sdOK)
  {
    safeCopy(sdStatusText, sizeof(sdStatusText), "OK");
    createLogHeaderIfNeeded();
    Serial.println("SD card OK");
  }
  else
  {
    safeCopy(sdStatusText, sizeof(sdStatusText), "FAILED");
    Serial.println("SD card failed");
  }
}

bool tryStartSD(uint32_t frequency)
{
  digitalWrite(BIG_TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  SD.end();
  delay(80);
  bool ok = SD.begin(SD_CS, sdSPI, frequency, "/sd", 5);
  digitalWrite(SD_CS, HIGH);
  return ok;
}

// ------------------------------------------------------------
// Updates
// ------------------------------------------------------------

void updateGNSS()
{
  while (GNSS.available())
  {
    char c = GNSS.read();
    gps.encode(c);
    gnssChars++;
    lastGnssCharTime = millis();

    if (c == '\n')
    {
      nmeaLine[nmeaIndex] = '\0';
      if (nmeaIndex > 5)
      {
        strncpy(lastNmeaLine, nmeaLine, sizeof(lastNmeaLine) - 1);
        lastNmeaLine[sizeof(lastNmeaLine) - 1] = '\0';
      }
      nmeaIndex = 0;
    }
    else if (c != '\r')
    {
      if (nmeaIndex < sizeof(nmeaLine) - 1)
      {
        nmeaLine[nmeaIndex++] = c;
      }
      else
      {
        nmeaIndex = 0;
      }
    }
  }
}

void updateSensors()
{
  if (millis() - lastSensorUpdate < 150) return;
  lastSensorUpdate = millis();

  if (bmpOK)
  {
    bmpTempC = bmp.readTemperature();
    bmpPressureHpa = bmp.readPressure() / 100.0F;
  }

  if (qmiOK && qmi.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::ACCEL)))
  {
    qmi.readAccel(acc);
    qmi.readGyro(gyr);
    accX = acc.mps2.x;
    accY = acc.mps2.y;
    accZ = acc.mps2.z;
    accMagnitude = sqrt((accX * accX) + (accY * accY) + (accZ * accZ));
  }

  if (millis() - lastSerialPrint > 2500)
  {
    lastSerialPrint = millis();
    Serial.print("Menu "); Serial.print((int)currentMenu);
    Serial.print(" | Fix "); Serial.print(gnssHasFix() ? "YES" : "NO");
    Serial.print(" | Sats "); Serial.print(gnssSatellites());
    Serial.print(" | HDOP "); Serial.print(gnssHdop(), 2);
    Serial.print(" | WiFi "); Serial.print(wifiStatusText);
    Serial.print(" | Server "); Serial.print(serverStatusText);
    Serial.print(" | HTTP "); Serial.print(lastHttpCode);
    Serial.print(" | SD "); Serial.print(sdStatusText);
    Serial.println();
  }
}

void updateWiFiNonBlocking()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiOK = true;
    safeCopy(wifiStatusText, sizeof(wifiStatusText), "OK");
    return;
  }

  wifiOK = false;
  safeCopy(wifiStatusText, sizeof(wifiStatusText), "OFFLINE");

  if (millis() - lastWiFiRetry >= WIFI_RETRY_INTERVAL_MS)
  {
    lastWiFiRetry = millis();
    Serial.print("Retrying WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.disconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    safeCopy(wifiStatusText, sizeof(wifiStatusText), "CONNECTING");
  }
}

void updateWebUpload()
{
  if (millis() - lastUploadTime < UPLOAD_INTERVAL_MS) return;
  lastUploadTime = millis();

  if (WiFi.status() != WL_CONNECTED)
  {
    serverOK = false;
    safeCopy(serverStatusText, sizeof(serverStatusText), "NO WIFI");
    return;
  }

  uploadTelemetry();
}

void updateSDLogging()
{
  if (!sdOK) return;
  if (millis() - lastLogTime < LOG_INTERVAL_MS) return;
  lastLogTime = millis();
  appendLogRow();
}

void updateTouch()
{
  uint16_t x, y;
  bool pressed = readMappedTouch(&x, &y);
  touchPressed = pressed;

  if (!pressed)
  {
    touchWasDown = false;
    return;
  }

  touchOK = true;
  touchX = x;
  touchY = y;

  if (touchWasDown) return;
  touchWasDown = true;

  if (millis() - lastTouchAction < 220) return;
  lastTouchAction = millis();

  processTouch(x, y);
}

// ------------------------------------------------------------
// Wi-Fi / backend
// ------------------------------------------------------------

void uploadTelemetry()
{
  StaticJsonDocument<768> doc;
  doc["device_id"] = DEVICE_ID;
  doc["mode"] = "geocache";

  if (gps.location.isValid())
  {
    doc["lat"] = gps.location.lat();
    doc["lon"] = gps.location.lng();
  }
  if (gps.altitude.isValid()) doc["alt_m"] = gps.altitude.meters();
  if (gps.speed.isValid()) doc["speed_kmph"] = gps.speed.kmph();

  doc["satellites"] = gnssSatellites();
  if (gps.hdop.isValid()) doc["hdop"] = gps.hdop.hdop();
  doc["fix"] = gnssHasFix();

  if (bmpOK)
  {
    doc["temperature_c"] = bmpTempC;
    doc["pressure_hpa"] = bmpPressureHpa;
  }

  if (qmiOK)
  {
    doc["acc_x"] = accX;
    doc["acc_y"] = accY;
    doc["acc_z"] = accZ;
  }

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.begin(TELEMETRY_URL);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(body);
  lastHttpCode = code;

  Serial.print("Telemetry HTTP: ");
  Serial.println(code);

  if (code >= 200 && code < 300)
  {
    String response = http.getString();
    serverOK = true;
    uploadCount++;
    safeCopy(serverStatusText, sizeof(serverStatusText), "OK");
    parseServerReply(response);
  }
  else
  {
    serverOK = false;
    uploadFailCount++;
    safeCopy(serverStatusText, sizeof(serverStatusText), "ERROR");
    if (code <= 0) Serial.println(http.errorToString(code));
  }

  http.end();
}

void parseServerReply(const String &response)
{
  StaticJsonDocument<1024> reply;
  DeserializationError err = deserializeJson(reply, response);
  if (err)
  {
    safeCopy(serverStatusText, sizeof(serverStatusText), "JSON ERR");
    Serial.print("Server JSON error: ");
    Serial.println(err.c_str());
    return;
  }

  safeCopy(serverCommand, sizeof(serverCommand), reply["command"] | "WAITING");
  safeCopy(serverColour, sizeof(serverColour), reply["screen_color"] | "yellow");
  safeCopy(serverMessage, sizeof(serverMessage), reply["message"] | "No message");
  const char *targetName = reply["target_name"] | nullptr;
  if (targetName == nullptr) targetName = reply["title"] | "No target";
  safeCopy(serverTarget, sizeof(serverTarget), targetName);
  serverDistanceM = reply["distance_m"] | -1.0f;

  bool doBeep = reply["beep"] | false;
  if (doBeep) beep();
}

void saveMarkedLocation()
{
  if (!gnssHasFix())
  {
    safeCopy(saveStatusText, sizeof(saveStatusText), "No GNSS fix. Cannot save.");
    beep();
    return;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    safeCopy(saveStatusText, sizeof(saveStatusText), "No WiFi. Cannot save.");
    beep();
    return;
  }

  StaticJsonDocument<512> doc;
  doc["device_id"] = DEVICE_ID;
  doc["asset_type"] = "marker";
  doc["name"] = "Marked spot from Sat Cacher";
  doc["lat"] = gps.location.lat();
  doc["lon"] = gps.location.lng();
  if (gps.altitude.isValid()) doc["alt_m"] = gps.altitude.meters();
  doc["description"] = "Saved by button press on the ESP32-S3 device. Rename this on the web dashboard.";
  doc["satellites"] = gnssSatellites();
  if (gps.hdop.isValid()) doc["hdop"] = gps.hdop.hdop();

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.begin(ASSETS_URL);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);

  Serial.print("Save asset HTTP: ");
  Serial.println(code);

  if (code >= 200 && code < 300)
  {
    String response = http.getString();
    StaticJsonDocument<768> reply;
    DeserializationError saveErr = deserializeJson(reply, response);
    if (!saveErr)
    {
      lastSavedAssetId = reply["asset"]["id"] | -1;
    }
    safeCopy(saveStatusText, sizeof(saveStatusText), "Saved. Rename on website.");
    beep();
  }
  else
  {
    safeCopy(saveStatusText, sizeof(saveStatusText), "Save failed. Check backend.");
    beep();
  }

  http.end();
  forceRedraw = true;
}

void safeCopy(char *dest, size_t destSize, const char *src)
{
  if (destSize == 0) return;
  if (src == nullptr)
  {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, destSize - 1);
  dest[destSize - 1] = '\0';
}

// ------------------------------------------------------------
// Touch and controls
// ------------------------------------------------------------

bool readMappedTouch(uint16_t *xOut, uint16_t *yOut)
{
  uint16_t rx, ry;
  if (!tft.getTouch(&rx, &ry)) return false;

  rawTouchX = rx;
  rawTouchY = ry;

  int x = (int)rx;
  int y = BIG_H - 1 - (int)ry;

#if TOUCH_INVERT_X
  x = BIG_W - 1 - x;
#endif

  x = constrain(x, 0, BIG_W - 1);
  y = constrain(y, 0, BIG_H - 1);

  *xOut = x;
  *yOut = y;
  return true;
}

void processTouch(uint16_t x, uint16_t y)
{
  if (y >= NAV_Y)
  {
    beep();
    if (x <= NAV_BACK_X1) previousMenu();
    else if (x <= NAV_HOME_X1) goHome();
    else nextMenu();
    return;
  }

  if (currentMenu == MENU_HOME)
  {
    for (int i = 0; i < 5; i++)
    {
      int boxY = 78 + i * 58;
      if (x >= 18 && x <= 302 && y >= boxY && y <= boxY + 42)
      {
        beep();
        currentMenu = (MenuPage)(i + 1);
        forceRedraw = true;
        return;
      }
    }
  }

  if (currentMenu == MENU_GEOCACHE)
  {
    if (x >= SAVE_BTN_X && x <= SAVE_BTN_X + SAVE_BTN_W &&
        y >= SAVE_BTN_Y && y <= SAVE_BTN_Y + SAVE_BTN_H)
    {
      saveMarkedLocation();
      return;
    }
  }
}

void handleSerialInput()
{
  while (Serial.available() > 0)
  {
    char c = Serial.read();
    if (c == 'n' || c == 'N' || c == '+') nextMenu();
    else if (c == 'p' || c == 'P' || c == '-') previousMenu();
    else if (c == 'h' || c == 'H' || c == '0') goHome();
    else if (c >= '1' && c <= '5')
    {
      currentMenu = (MenuPage)(c - '0');
      forceRedraw = true;
    }
    else if (c == 'u' || c == 'U') uploadTelemetry();
    else if (c == 'm' || c == 'M') saveMarkedLocation();
  }
}

void handleBootButton()
{
  if (digitalRead(BOOT_BUTTON_PIN) == LOW && millis() - lastBootPress > 450)
  {
    lastBootPress = millis();
    beep();
    nextMenu();
  }
}

void nextMenu()
{
  currentMenu = (MenuPage)(((int)currentMenu + 1) % MENU_COUNT);
  forceRedraw = true;
}

void previousMenu()
{
  int m = (int)currentMenu - 1;
  if (m < 0) m = MENU_COUNT - 1;
  currentMenu = (MenuPage)m;
  forceRedraw = true;
}

void goHome()
{
  currentMenu = MENU_HOME;
  forceRedraw = true;
}

// ------------------------------------------------------------
// Display drawing
// ------------------------------------------------------------

void drawScreen()
{
  tft.fillScreen(TFT_BLACK);

  switch (currentMenu)
  {
    case MENU_HOME: drawHomeMenu(); break;
    case MENU_LIVE_GNSS: drawLiveGNSSMenu(); break;
    case MENU_SIGNAL_QUALITY: drawSignalQualityMenu(); break;
    case MENU_FSPL: drawFSPLMenu(); break;
    case MENU_SYSTEM: drawSystemMenu(); break;
    case MENU_GEOCACHE: drawGeocacheMenu(); break;
    default: drawHomeMenu(); break;
  }
}

void updateScreenValues()
{
  switch (currentMenu)
  {
    case MENU_HOME: updateHomeValues(); break;
    case MENU_LIVE_GNSS: updateLiveGNSSValues(); break;
    case MENU_SIGNAL_QUALITY: updateSignalQualityValues(); break;
    case MENU_SYSTEM: updateSystemValues(); break;
    case MENU_GEOCACHE: updateGeocacheValues(); break;
    default: break;
  }
}

void drawHeader(const char *title)
{
  tft.fillRect(0, 0, BIG_W, 52, TFT_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setCursor(10, 17);
  tft.print(title);
  drawNavBar();
}

void drawNavBar()
{
  tft.fillRect(0, NAV_Y, BIG_W, NAV_H, TFT_DARKGREY);
  tft.drawFastVLine(NAV_BACK_X1, NAV_Y, NAV_H, TFT_BLACK);
  tft.drawFastVLine(NAV_HOME_X1, NAV_Y, NAV_H, TFT_BLACK);
  drawBackIcon(53, 445, TFT_WHITE);
  drawHomeIcon(160, 445, TFT_WHITE);
  drawNextIcon(267, 445, TFT_WHITE);
}

void drawBackIcon(int cx, int cy, uint16_t colour)
{
  tft.fillTriangle(cx - 16, cy, cx + 5, cy - 14, cx + 5, cy + 14, colour);
  tft.fillRect(cx + 4, cy - 6, 20, 12, colour);
}

void drawNextIcon(int cx, int cy, uint16_t colour)
{
  tft.fillTriangle(cx + 16, cy, cx - 5, cy - 14, cx - 5, cy + 14, colour);
  tft.fillRect(cx - 24, cy - 6, 20, 12, colour);
}

void drawHomeIcon(int cx, int cy, uint16_t colour)
{
  tft.fillTriangle(cx - 18, cy - 2, cx, cy - 20, cx + 18, cy - 2, colour);
  tft.fillRect(cx - 13, cy - 2, 26, 22, colour);
  tft.fillRect(cx - 4, cy + 7, 8, 13, TFT_DARKGREY);
}

void clearValueArea(int x, int y, int w, int h)
{
  tft.fillRect(x, y, w, h, TFT_BLACK);
}

void drawLabelValue(int x, int y, const char *label, const char *value, uint16_t valueColour)
{
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(x, y);
  tft.print(label);
  tft.setTextColor(valueColour, TFT_BLACK);
  tft.print(value);
}

void drawHomeMenu()
{
  drawHeader("SAT CACHER");

  const char *items[] = {
    "Live GNSS",
    "Signal Quality",
    "FSPL Calculator",
    "System Status",
    "Geocache"
  };

  tft.setTextSize(2);
  for (int i = 0; i < 5; i++)
  {
    int y = 78 + i * 58;
    tft.drawRoundRect(18, y, 284, 42, 8, TFT_DARKGREY);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(31, y + 13);
    tft.print(i + 1);
    tft.print(". ");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print(items[i]);
  }

  updateHomeValues();
}

void updateHomeValues()
{
  tft.fillRect(18, 365, 286, 36, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(18, 365);
  tft.setTextColor(qualityColour(), TFT_BLACK);
  tft.print("GNSS: "); tft.print(qualityText());
  tft.print(" | Sats: "); tft.print(gnssSatellites());

  tft.setCursor(18, 383);
  tft.setTextColor(wifiOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.print("WiFi: "); tft.print(wifiStatusText);
  tft.setTextColor(serverOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.print(" | Server: "); tft.print(serverStatusText);
}

void drawLiveGNSSMenu()
{
  drawHeader("LIVE GNSS");
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 72);  tft.print("Fix:");
  tft.setCursor(20, 108); tft.print("Lat:");
  tft.setCursor(20, 144); tft.print("Lon:");
  tft.setCursor(20, 180); tft.print("Alt:");
  tft.setCursor(20, 216); tft.print("Speed:");
  tft.setCursor(20, 252); tft.print("Sats:");
  tft.setCursor(20, 288); tft.print("HDOP:");
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 334); tft.print("Last raw NMEA:");
  updateLiveGNSSValues();
}

void updateLiveGNSSValues()
{
  tft.fillRect(112, 68, 198, 250, TFT_BLACK);
  tft.setTextSize(2);

  tft.setCursor(112, 72);
  tft.setTextColor(gnssHasFix() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.print(gnssHasFix() ? "YES" : "NO");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(112, 108);
  if (gps.location.isValid()) tft.print(gps.location.lat(), 6); else tft.print("--");
  tft.setCursor(112, 144);
  if (gps.location.isValid()) tft.print(gps.location.lng(), 6); else tft.print("--");
  tft.setCursor(112, 180);
  if (gps.altitude.isValid()) { tft.print(gps.altitude.meters(), 1); tft.print("m"); } else tft.print("--");
  tft.setCursor(112, 216);
  if (gps.speed.isValid()) { tft.print(gps.speed.kmph(), 1); tft.print("km/h"); } else tft.print("--");
  tft.setCursor(112, 252);
  tft.print(gnssSatellites()); tft.print(" visible");
  tft.setCursor(112, 288);
  if (gps.hdop.isValid()) tft.print(gnssHdop(), 2); else tft.print("--");

  tft.fillRect(18, 352, 286, 48, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(20, 352);
  char shortNmea[78];
  strncpy(shortNmea, lastNmeaLine, sizeof(shortNmea) - 1);
  shortNmea[sizeof(shortNmea) - 1] = '\0';
  tft.print(shortNmea);
}

void drawSignalQualityMenu()
{
  drawHeader("SIGNAL QUALITY");
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 78);  tft.print("Status:");
  tft.setCursor(20, 176); tft.print("Fix:");
  tft.setCursor(20, 212); tft.print("Sats:");
  tft.setCursor(20, 248); tft.print("HDOP:");
  tft.setCursor(20, 304); tft.print("Reason:");
  updateSignalQualityValues();
}

void updateSignalQualityValues()
{
  tft.fillRect(20, 108, 286, 286, TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(qualityColour(), TFT_BLACK);
  tft.setCursor(20, 112);
  tft.print(qualityText());

  tft.setTextSize(2);
  tft.setTextColor(gnssHasFix() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.setCursor(115, 176); tft.print(gnssHasFix() ? "YES" : "NO");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(115, 212); tft.print(gnssSatellites());
  tft.setCursor(115, 248); if (gps.hdop.isValid()) tft.print(gnssHdop(), 2); else tft.print("--");
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(20, 334);
  tft.print(qualityReason());
}

void drawFSPLMenu()
{
  drawHeader("FSPL CALC");

  const float L1 = 1575.42f;
  const float L5 = 1176.45f;
  const float distKm[] = {0.01f, 0.1f, 1.0f, 10.0f};
  const char *labels[] = {"10 m", "100 m", "1 km", "10 km"};

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 70);
  tft.print("Free Space Path Loss examples");
  tft.setCursor(20, 90);
  tft.print("L1 1575.42 MHz, L5 1176.45 MHz");

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 130); tft.print("Dist");
  tft.setCursor(120, 130); tft.print("L1 dB");
  tft.setCursor(225, 130); tft.print("L5 dB");

  for (int i = 0; i < 4; i++)
  {
    int y = 175 + i * 45;
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(20, y); tft.print(labels[i]);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(120, y); tft.print(calculateFSPL(distKm[i], L1), 1);
    tft.setCursor(225, y); tft.print(calculateFSPL(distKm[i], L5), 1);
  }

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 370);
  tft.print("This is theoretical path loss only.");
}

void drawSystemMenu()
{
  drawHeader("SYSTEM STATUS");
  updateSystemValues();
}

void updateSystemValues()
{
  tft.fillRect(12, 62, 300, 340, TFT_BLACK);
  tft.setTextSize(1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int y = 68;
  tft.setCursor(18, y); tft.print("BMP280: "); tft.setTextColor(bmpOK ? TFT_GREEN : TFT_RED, TFT_BLACK); tft.print(bmpOK ? "OK" : "NO");
  y += 20; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("QMI8658C: "); tft.setTextColor(qmiOK ? TFT_GREEN : TFT_RED, TFT_BLACK); tft.print(qmiOK ? "OK" : "NO");
  y += 20; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("SD Card: "); tft.setTextColor(sdOK ? TFT_GREEN : TFT_RED, TFT_BLACK); tft.print(sdStatusText);
  y += 20; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("Touch: "); tft.setTextColor(touchOK ? TFT_GREEN : TFT_YELLOW, TFT_BLACK); tft.print(touchOK ? "OK" : "WAIT");
  y += 20; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("GNSS UART: "); tft.setTextColor(gnssReceiving() ? TFT_GREEN : TFT_RED, TFT_BLACK); tft.print(gnssReceiving() ? "OK" : "NO DATA");

  y += 32; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("WiFi: "); tft.setTextColor(wifiOK ? TFT_GREEN : TFT_RED, TFT_BLACK); tft.print(wifiStatusText);
  y += 20; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("ESP32 IP: "); printIPAddressToScreen(90, y);
  y += 20; tft.setCursor(18, y); tft.print("Server IP: "); tft.print(SERVER_HOST);
  y += 20; tft.setCursor(18, y); tft.print("Server: "); tft.setTextColor(serverOK ? TFT_GREEN : TFT_RED, TFT_BLACK); tft.print(serverStatusText);
  y += 20; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("HTTP: "); tft.print(lastHttpCode); tft.print("  Uploads: "); tft.print(uploadCount);

  y += 32; tft.setCursor(18, y); tft.print("Temp: "); tft.setTextColor(thermalColour(), TFT_BLACK); tft.print(bmpTempC, 1); tft.print(" C "); tft.print(thermalStatus());
  y += 20; tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(18, y); tft.print("Pressure: "); tft.print(bmpPressureHpa, 1); tft.print(" hPa");
  y += 20; tft.setCursor(18, y); tft.print("AX:"); tft.print(accX, 2); tft.print(" AY:"); tft.print(accY, 2); tft.print(" AZ:"); tft.print(accZ, 2);
  y += 20; tft.setCursor(18, y); tft.print("Logs: "); tft.print(logRows); tft.print("  File: "); tft.print(LOG_FILE);
  y += 20; tft.setCursor(18, y); tft.print("Touch raw: "); tft.print(rawTouchX); tft.print(","); tft.print(rawTouchY);
}

void drawGeocacheMenu()
{
  drawHeader("GEOCACHE");

  tft.drawRoundRect(SAVE_BTN_X, SAVE_BTN_Y, SAVE_BTN_W, SAVE_BTN_H, 8, TFT_CYAN);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(45, SAVE_BTN_Y + 14);
  tft.print("Save marked location");

  updateGeocacheValues();
}

void updateGeocacheValues()
{
  tft.fillRect(18, 65, 286, 250, TFT_BLACK);

  uint16_t boxColour = colourFromServer(serverColour);
  tft.fillRoundRect(18, 70, 284, 54, 8, boxColour);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, boxColour);
  tft.setCursor(30, 88);
  tft.print(serverCommand);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 140);
  tft.print("Target:");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(80, 140);
  tft.print(serverTarget);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 165);
  tft.print("Distance:");
  tft.setCursor(90, 165);
  if (serverDistanceM >= 0)
  {
    if (serverDistanceM >= 1000.0f)
    {
      tft.print(serverDistanceM / 1000.0f, 2);
      tft.print(" km");
    }
    else
    {
      tft.print(serverDistanceM, 1);
      tft.print(" m");
    }
  }
  else
  {
    tft.print("--");
  }

  tft.setCursor(20, 198);
  tft.print("Message:");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 218);
  char msg[92];
  strncpy(msg, serverMessage, sizeof(msg) - 1);
  msg[sizeof(msg) - 1] = '\0';
  tft.print(msg);

  tft.fillRect(18, 383, 286, 20, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(20, 386);
  tft.print(saveStatusText);
}

void printIPAddressToScreen(int x, int y)
{
  tft.setCursor(x, y);
  if (WiFi.status() == WL_CONNECTED)
  {
    tft.print(WiFi.localIP());
  }
  else
  {
    tft.print("-");
  }
}

// ------------------------------------------------------------
// SD logging
// ------------------------------------------------------------

void createLogHeaderIfNeeded()
{
  if (!sdOK) return;

  bool exists = SD.exists(LOG_FILE);
  File file = SD.open(LOG_FILE, FILE_APPEND);
  if (!file)
  {
    sdOK = false;
    safeCopy(sdStatusText, sizeof(sdStatusText), "FILE FAIL");
    return;
  }

  if (!exists || file.size() == 0)
  {
    file.println("millis,fix,lat,lon,alt_m,speed_kmph,satellites,hdop,quality,temp_c,pressure_hpa,acc_x,acc_y,acc_z,wifi,server,http_code,command,target,distance_m");
  }
  file.close();
}

void appendLogRow()
{
  File file = SD.open(LOG_FILE, FILE_APPEND);
  if (!file)
  {
    sdOK = false;
    safeCopy(sdStatusText, sizeof(sdStatusText), "LOG FAIL");
    return;
  }

  file.print(millis()); file.print(',');
  file.print(gnssHasFix() ? "YES" : "NO"); file.print(',');
  printFloatOrBlank(file, gps.location.isValid(), gps.location.lat(), 6); file.print(',');
  printFloatOrBlank(file, gps.location.isValid(), gps.location.lng(), 6); file.print(',');
  printFloatOrBlank(file, gps.altitude.isValid(), gps.altitude.meters(), 1); file.print(',');
  printFloatOrBlank(file, gps.speed.isValid(), gps.speed.kmph(), 1); file.print(',');
  file.print(gnssSatellites()); file.print(',');
  printFloatOrBlank(file, gps.hdop.isValid(), gnssHdop(), 2); file.print(',');
  file.print(qualityText()); file.print(',');
  printFloatOrBlank(file, bmpOK, bmpTempC, 1); file.print(',');
  printFloatOrBlank(file, bmpOK, bmpPressureHpa, 1); file.print(',');
  printFloatOrBlank(file, qmiOK, accX, 3); file.print(',');
  printFloatOrBlank(file, qmiOK, accY, 3); file.print(',');
  printFloatOrBlank(file, qmiOK, accZ, 3); file.print(',');
  file.print(wifiStatusText); file.print(',');
  file.print(serverStatusText); file.print(',');
  file.print(lastHttpCode); file.print(',');
  file.print(serverCommand); file.print(',');
  file.print(serverTarget); file.print(',');
  if (serverDistanceM >= 0) file.print(serverDistanceM, 1);
  file.println();
  file.close();
  logRows++;
}

void printFloatOrBlank(File &file, bool valid, double value, int decimals)
{
  if (valid) file.print(value, decimals);
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

bool gnssReceiving()
{
  return millis() - lastGnssCharTime < 3000;
}

bool gnssHasFix()
{
  return gps.location.isValid() && gps.location.age() < 5000;
}

int gnssSatellites()
{
  if (gps.satellites.isValid()) return gps.satellites.value();
  return 0;
}

float gnssHdop()
{
  if (gps.hdop.isValid()) return gps.hdop.hdop();
  return 99.99f;
}

const char *qualityText()
{
  if (!gnssReceiving()) return "NO DATA";
  if (!gnssHasFix()) return "POOR";
  if (gnssSatellites() >= 8 && gnssHdop() <= 2.0f) return "GOOD";
  if (gnssSatellites() >= 4 && gnssHdop() <= 5.0f) return "OK";
  return "POOR";
}

const char *qualityReason()
{
  if (!gnssReceiving()) return "No NMEA data from LC29H";
  if (!gnssHasFix()) return "Waiting for a valid GNSS fix";
  if (gnssSatellites() < 4) return "Too few satellites";
  if (gnssHdop() > 5.0f) return "HDOP is high";
  if (gnssSatellites() >= 8 && gnssHdop() <= 2.0f) return "Strong fix and low HDOP";
  return "Usable fix, but not excellent";
}

uint16_t qualityColour()
{
  const char *q = qualityText();
  if (strcmp(q, "GOOD") == 0) return TFT_GREEN;
  if (strcmp(q, "OK") == 0) return TFT_YELLOW;
  if (strcmp(q, "NO DATA") == 0) return TFT_ORANGE;
  return TFT_RED;
}

uint16_t colourFromServer(const char *name)
{
  if (strcmp(name, "green") == 0) return TFT_GREEN;
  if (strcmp(name, "red") == 0) return TFT_RED;
  if (strcmp(name, "blue") == 0) return TFT_BLUE;
  if (strcmp(name, "orange") == 0) return TFT_ORANGE;
  if (strcmp(name, "yellow") == 0) return TFT_YELLOW;
  return TFT_DARKGREY;
}

const char *thermalStatus()
{
  if (!bmpOK) return "NO BMP";
  if (bmpTempC < 50.0f) return "OK";
  if (bmpTempC < 60.0f) return "WARM";
  return "HOT";
}

uint16_t thermalColour()
{
  if (!bmpOK) return TFT_RED;
  if (bmpTempC < 50.0f) return TFT_GREEN;
  if (bmpTempC < 60.0f) return TFT_YELLOW;
  return TFT_RED;
}

float calculateFSPL(float distanceKm, float frequencyMHz)
{
  return 32.44f + 20.0f * log10(distanceKm) + 20.0f * log10(frequencyMHz);
}

void beep()
{
  tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_BEEP_MS);
}

void startupBeep()
{
  tone(BUZZER_PIN, 900, 40);
  delay(70);
  tone(BUZZER_PIN, 1250, 40);
  delay(70);
}
