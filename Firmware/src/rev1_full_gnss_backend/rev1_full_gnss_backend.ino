/********************************************************
  Tenstar ESP32-S3 GNSS Field Tool + FastAPI Upload

  Features:
  - External ILI9488-compatible display using LovyanGFX
  - XPT2046 touch
  - LC29H GNSS using TinyGPSPlus
  - SD card logging
  - BMP280 temperature/pressure
  - QMI8658C accelerometer/gyro
  - Buzzer feedback
  - Wi-Fi upload to FastAPI backend
  - Parses server command response

  Required Arduino libraries:
  - LovyanGFX
  - TinyGPSPlus
  - Adafruit BMP280
  - ArduinoJson
  - QMI8658 driver file: ImuDrv.hpp
********************************************************/

// Current Issues:
/*
1. Gets stuck in connecting to wifi due to polling
2. Touch becomes unrepsonsive
3. Need RTOS or scheduler for each function
4. Slow uploads 
5. Needs Testing!
6. Need to make menu for GeoCache to change the screen color and give hints 
*/

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
// Wi-Fi + FastAPI upload
// ------------------------------------------------------------

const char *WIFI_SSID = "yep";
const char *WIFI_PASSWORD = "12345678";

// Laptop local IP.
// ESP32 must use laptop IP, not localhost or 127.0.0.1.
const char *SERVER_URL = "http://10.107.7.42:8000/api/telemetry";

#define UPLOAD_INTERVAL_MS       5000
#define WIFI_CONNECT_TIMEOUT_MS  15000
#define WIFI_RETRY_INTERVAL_MS   10000
#define HTTP_TIMEOUT_MS          5000

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

#define LOG_FILE "/gnss_log.csv"
#define LOG_INTERVAL_MS 5000

// ------------------------------------------------------------
// Buzzer
// ------------------------------------------------------------

#define BUZZER_PIN 5
#define BUZZER_FREQ 1300
#define BUZZER_BEEP_MS 18

// ------------------------------------------------------------
// Display
// ------------------------------------------------------------

#define BIG_W 320
#define BIG_H 480

#define DISPLAY_ROTATION 2
#define DISPLAY_UPDATE_MS 1500

// ------------------------------------------------------------
// Touch mapping
// ------------------------------------------------------------

/*
  Your touch readings showed:
  raw: 271,45
  raw: 53,37
  raw: 148,39
  raw: 244,68

  Correct mapping:
  x = rawX
  y = 479 - rawY

  This puts the bottom buttons inside y = 410 to 479.
*/

#define TOUCH_INVERT_X false

// ------------------------------------------------------------
// Navigation bar
// ------------------------------------------------------------

#define NAV_Y 410
#define NAV_H 70

#define NAV_BACK_X0 0
#define NAV_BACK_X1 106

#define NAV_HOME_X0 107
#define NAV_HOME_X1 213

#define NAV_NEXT_X0 214
#define NAV_NEXT_X1 319

#define BOOT_BUTTON_PIN 0

// ------------------------------------------------------------
// LovyanGFX display/touch class
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

LGFX_ILI9488_Touch bigTft;
SPIClass sdSPI(FSPI);

Adafruit_BMP280 bmp;
SensorQMI8658 qmi;

AccelerometerData acc;
GyroscopeData gyr;

HardwareSerial GNSS(1);
TinyGPSPlus gps;

// ------------------------------------------------------------
// Sensor / GNSS / SD state
// ------------------------------------------------------------

bool bmpOK = false;
bool qmiOK = false;
bool sdOK = false;

char sdStatusText[24] = "NOT STARTED";

float bmpTempC = 0.0;
float bmpPressureHpa = 0.0;

uint8_t qmiAddress = 0;

float accX = 0.0;
float accY = 0.0;
float accZ = 0.0;
float accMagnitude = 0.0;

uint32_t gnssChars = 0;
uint32_t gnssSentences = 0;
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

unsigned long lastTouchAction = 0;
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastBootPress = 0;
unsigned long lastLogTime = 0;

uint32_t logRows = 0;

// ------------------------------------------------------------
// Web upload state
// ------------------------------------------------------------

unsigned long lastUploadTime = 0;
unsigned long lastWiFiAttemptTime = 0;

bool wifiOK = false;
bool serverOK = false;

int lastHttpCode = 0;
uint32_t uploadCount = 0;
uint32_t uploadFailCount = 0;

char uploadStatusText[32] = "NOT SENT";
char serverCommandText[32] = "-";
char serverScreenColour[20] = "-";
char serverTitleText[64] = "-";
char serverMessageText[160] = "-";

float serverDistanceM = -1.0f;

// ------------------------------------------------------------
// Menu system
// ------------------------------------------------------------

enum MenuPage
{
  MENU_HOME = 0,
  MENU_LIVE_GNSS,
  MENU_SATELLITES,
  MENU_SIGNAL_QUALITY,
  MENU_FSPL,
  MENU_CAMPUS_SURVEY,
  MENU_SYSTEM,
  MENU_COUNT
};

MenuPage currentMenu = MENU_HOME;
bool forceRedraw = true;

// ------------------------------------------------------------
// Prototypes
// ------------------------------------------------------------

void initDisplay();
void initSensors();
void initGNSS();
void initSDCard();
bool tryStartSD(uint32_t frequency);
void initBuzzer();
void gentleBeep();

void updateSensors();
void updateGNSS();
void updateTouch();
void updateSDLogging();
void updateWebUpload();

void connectWiFi(bool forceAttempt = false);
void uploadTelemetry();
void parseServerReply(const String &response);
void safeCopy(char *dest, size_t destSize, const char *src);

void handleSerialInput();
void handleBootButton();

void drawDisplay();
void updateDisplayValues();

void drawHeader(const char *title);
void drawNavBar();
void drawBackIcon(int cx, int cy, uint16_t colour);
void drawHomeIcon(int cx, int cy, uint16_t colour);
void drawNextIcon(int cx, int cy, uint16_t colour);

void drawHomeMenu();
void drawLiveGNSSMenu();
void drawSatellitesMenu();
void drawSignalQualityMenu();
void drawFSPLMenu();
void drawCampusSurveyMenu();
void drawSystemMenu();

void updateHomeValues();
void updateLiveGNSSValues();
void updateSatelliteValues();
void updateSignalQualityValues();
void updateCampusSurveyValues();
void updateSystemValues();

bool readMappedTouch(uint16_t *xOut, uint16_t *yOut);
void processTouch(uint16_t x, uint16_t y);

void nextMenu();
void previousMenu();
void goHome();

bool gnssReceiving();
bool gnssHasFix();
int gnssSatellites();
float gnssHdop();

const char *qualityText();
uint16_t qualityColour();
const char *qualityReason();

const char *thermalStatus();
uint16_t thermalColour();

void createLogHeaderIfNeeded();
void appendLogRow();
void printFloatOrBlank(File &file, bool valid, double value, int decimals);

float calculateFSPL(float distanceKm, float frequencyMHz);

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("====================================");
  Serial.println("Tenstar ESP32-S3 GNSS + Web Upload");
  Serial.println("====================================");
  Serial.println("Display: ILI9488-compatible, rotation 2");
  Serial.println("Touch: x = rawX, y = 479 - rawY");
  Serial.println("LC29H: RX GPIO17, TX GPIO18");
  Serial.println("SD: SCK36 MOSI35 MISO37 CS9");
  Serial.println("WiFi SSID: yep");
  Serial.print("Server URL: ");
  Serial.println(SERVER_URL);

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  pinMode(BIG_TFT_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(BIG_TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  initBuzzer();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  initDisplay();
  initSensors();
  initGNSS();
  initSDCard();

  connectWiFi(true);

  updateSensors();
  updateGNSS();

  drawDisplay();

  Serial.println("Setup complete");
  Serial.println("Controls: n=next, p=previous, h=home, 1-6=direct page");
}

// ------------------------------------------------------------
// Loop
// ------------------------------------------------------------

void loop()
{
  handleSerialInput();
  handleBootButton();

  updateGNSS();
  updateTouch();
  updateSensors();
  updateSDLogging();
  updateWebUpload();

  if (forceRedraw)
  {
    drawDisplay();
    forceRedraw = false;
    lastDisplayUpdate = millis();
  }

  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_MS)
  {
    lastDisplayUpdate = millis();
    updateDisplayValues();
  }
}

// ------------------------------------------------------------
// Init functions
// ------------------------------------------------------------

void initBuzzer()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void gentleBeep()
{
  tone(BUZZER_PIN, BUZZER_FREQ, BUZZER_BEEP_MS);
}

void initDisplay()
{
  Serial.println("Starting external ILI9488 + XPT2046 touch");

  bool ok = bigTft.init();

  Serial.print("ILI9488 init result: ");
  Serial.println(ok ? "OK" : "FAILED");

  bigTft.setRotation(DISPLAY_ROTATION);
  bigTft.fillScreen(TFT_BLACK);

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setTextSize(2);
  bigTft.setCursor(20, 200);
  bigTft.print("DISPLAY OK");

  delay(400);

  Serial.println("Display ready");
}

void initSensors()
{
  Serial.println("Initializing BMP280");

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

  Serial.println("Initializing QMI8658C");

  if (qmi.begin(Wire, QMI_ADDR_1, I2C_SDA, I2C_SCL))
  {
    qmiOK = true;
    qmiAddress = QMI_ADDR_1;
    Serial.println("QMI8658C found at 0x6B");
  }
  else if (qmi.begin(Wire, QMI_ADDR_2, I2C_SDA, I2C_SCL))
  {
    qmiOK = true;
    qmiAddress = QMI_ADDR_2;
    Serial.println("QMI8658C found at 0x6A");
  }
  else
  {
    qmiOK = false;
    Serial.println("QMI8658C not found");
    return;
  }

  Serial.print("QMI firmware version: 0x");
  Serial.println(qmi.getFirmwareVersion(), HEX);

  qmi.configAccel(
    AccelFullScaleRange::FS_8G,
    1000.0f,
    SensorQMI8658::LpfMode::MODE_0
  );

  qmi.configGyro(
    GyroFullScaleRange::FS_1000_DPS,
    1000.0f,
    SensorQMI8658::LpfMode::MODE_0
  );

  qmi.enableAccel();
  qmi.enableGyro();

  Serial.println("QMI8658C setup complete");
}

void initGNSS()
{
  Serial.println("Starting LC29H UART");

  GNSS.setRxBufferSize(2048);
  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);

  Serial.print("LC29H UART started at ");
  Serial.print(GNSS_BAUD);
  Serial.println(" baud");

  Serial.println("LC29H TX -> GPIO17");
  Serial.println("LC29H RX -> GPIO18");
}

void initSDCard()
{
  Serial.println("Starting SD card");
  Serial.println("Expected SD wiring:");
  Serial.println("SD SCK  -> GPIO36");
  Serial.println("SD MOSI -> GPIO35");
  Serial.println("SD MISO -> GPIO37");
  Serial.println("SD CS   -> GPIO9");

  digitalWrite(BIG_TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  delay(200);

  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  Serial.println("Trying SD at 400 kHz...");
  sdOK = tryStartSD(SD_FREQ_SLOW);

  if (!sdOK)
  {
    Serial.println("400 kHz failed. Trying SD at 1 MHz...");
    sdOK = tryStartSD(SD_FREQ_MED);
  }

  if (!sdOK)
  {
    Serial.println("1 MHz failed. Trying SD at 4 MHz...");
    sdOK = tryStartSD(SD_FREQ_FAST);
  }

  if (!sdOK)
  {
    strcpy(sdStatusText, "FAILED");

    Serial.println("SD card FAILED");
    Serial.println("Check FAT32, CS GPIO9, SD power, and MISO/MOSI direction.");
    return;
  }

  strcpy(sdStatusText, "OK");

  Serial.println("SD card OK");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.print("SD card size: ");
  Serial.print(cardSize);
  Serial.println(" MB");

  createLogHeaderIfNeeded();
}

bool tryStartSD(uint32_t frequency)
{
  digitalWrite(BIG_TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  SD.end();
  delay(100);

  bool ok = SD.begin(SD_CS, sdSPI, frequency, "/sd", 5);

  digitalWrite(SD_CS, HIGH);

  return ok;
}

// ------------------------------------------------------------
// Wi-Fi + upload
// ------------------------------------------------------------

void connectWiFi(bool forceAttempt)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiOK = true;
    return;
  }

  wifiOK = false;

  if (!forceAttempt && millis() - lastWiFiAttemptTime < WIFI_RETRY_INTERVAL_MS)
  {
    return;
  }

  lastWiFiAttemptTime = millis();

  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS)
  {
    delay(500);
    Serial.print(".");
    updateGNSS();
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    wifiOK = true;
    Serial.println("WiFi connected");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    wifiOK = false;
    Serial.println("WiFi connection failed");
  }
}

void updateWebUpload()
{
  if (millis() - lastUploadTime < UPLOAD_INTERVAL_MS)
  {
    return;
  }

  lastUploadTime = millis();

  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi(false);
    return;
  }

  uploadTelemetry();
}

void uploadTelemetry()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    wifiOK = false;
    safeCopy(uploadStatusText, sizeof(uploadStatusText), "NO WIFI");
    return;
  }

  wifiOK = true;

  StaticJsonDocument<768> doc;

  doc["device_id"] = "tenstar_001";
  doc["mode"] = "geocache";

  if (gps.location.isValid())
  {
    doc["lat"] = gps.location.lat();
    doc["lon"] = gps.location.lng();
  }

  if (gps.altitude.isValid())
  {
    doc["alt_m"] = gps.altitude.meters();
  }

  if (gps.speed.isValid())
  {
    doc["speed_kmph"] = gps.speed.kmph();
  }

  if (gps.satellites.isValid())
  {
    doc["satellites"] = gps.satellites.value();
  }
  else
  {
    doc["satellites"] = 0;
  }

  if (gps.hdop.isValid())
  {
    doc["hdop"] = gps.hdop.hdop();
  }

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

  Serial.println();
  Serial.println("Uploading telemetry to FastAPI:");
  Serial.println(body);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(body);

  lastHttpCode = httpCode;

  Serial.print("HTTP code: ");
  Serial.println(httpCode);

  if (httpCode > 0)
  {
    String response = http.getString();

    Serial.println("Server response:");
    Serial.println(response);

    if (httpCode >= 200 && httpCode < 300)
    {
      serverOK = true;
      uploadCount++;
      safeCopy(uploadStatusText, sizeof(uploadStatusText), "OK");
      parseServerReply(response);
    }
    else
    {
      serverOK = false;
      uploadFailCount++;
      safeCopy(uploadStatusText, sizeof(uploadStatusText), "HTTP ERR");
    }
  }
  else
  {
    serverOK = false;
    uploadFailCount++;
    safeCopy(uploadStatusText, sizeof(uploadStatusText), "POST FAIL");

    Serial.print("HTTP POST failed: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

void parseServerReply(const String &response)
{
  StaticJsonDocument<1024> reply;

  DeserializationError err = deserializeJson(reply, response);

  if (err)
  {
    Serial.print("Server JSON parse failed: ");
    Serial.println(err.c_str());
    safeCopy(uploadStatusText, sizeof(uploadStatusText), "JSON FAIL");
    return;
  }

  const char *command = reply["command"] | "-";
  const char *screenColour = reply["screen_color"] | "-";
  const char *title = reply["title"] | "-";
  const char *message = reply["message"] | "-";

  serverDistanceM = reply["distance_m"] | -1.0;

  safeCopy(serverCommandText, sizeof(serverCommandText), command);
  safeCopy(serverScreenColour, sizeof(serverScreenColour), screenColour);
  safeCopy(serverTitleText, sizeof(serverTitleText), title);
  safeCopy(serverMessageText, sizeof(serverMessageText), message);

  Serial.println("Parsed server command:");
  Serial.print("Command: ");
  Serial.println(serverCommandText);
  Serial.print("Screen colour: ");
  Serial.println(serverScreenColour);
  Serial.print("Title: ");
  Serial.println(serverTitleText);
  Serial.print("Message: ");
  Serial.println(serverMessageText);
  Serial.print("Distance: ");
  Serial.println(serverDistanceM);

  bool beep = reply["beep"] | false;

  if (beep)
  {
    gentleBeep();
  }
}

void safeCopy(char *dest, size_t destSize, const char *src)
{
  if (destSize == 0)
  {
    return;
  }

  if (src == nullptr)
  {
    dest[0] = '\0';
    return;
  }

  strncpy(dest, src, destSize - 1);
  dest[destSize - 1] = '\0';
}

// ------------------------------------------------------------
// Updates
// ------------------------------------------------------------

void updateSensors()
{
  if (millis() - lastSensorUpdate < 100)
  {
    return;
  }

  lastSensorUpdate = millis();

  if (bmpOK)
  {
    bmpTempC = bmp.readTemperature();
    bmpPressureHpa = bmp.readPressure() / 100.0F;
  }

  if (qmiOK)
  {
    if (qmi.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::ACCEL)))
    {
      qmi.readAccel(acc);
      qmi.readGyro(gyr);

      accX = acc.mps2.x;
      accY = acc.mps2.y;
      accZ = acc.mps2.z;

      accMagnitude = sqrt((accX * accX) + (accY * accY) + (accZ * accZ));
    }
  }

  if (millis() - lastSerialPrint >= 2000)
  {
    lastSerialPrint = millis();

    Serial.print("Menu: ");
    Serial.print((int)currentMenu);

    Serial.print(" | GNSS chars: ");
    Serial.print(gnssChars);

    Serial.print(" | Sats: ");
    Serial.print(gnssSatellites());

    Serial.print(" | Fix: ");
    Serial.print(gnssHasFix() ? "YES" : "NO");

    Serial.print(" | Quality: ");
    Serial.print(qualityText());

    Serial.print(" | SD: ");
    Serial.print(sdOK ? "OK" : "NO");

    Serial.print(" | Logs: ");
    Serial.print(logRows);

    Serial.print(" | WiFi: ");
    Serial.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");

    Serial.print(" | Server: ");
    Serial.print(uploadStatusText);

    Serial.print(" | HTTP: ");
    Serial.print(lastHttpCode);

    Serial.print(" | Uploads: ");
    Serial.print(uploadCount);

    Serial.print(" | Board Temp: ");
    Serial.print(bmpTempC, 1);
    Serial.print(" C");

    if (touchPressed)
    {
      Serial.print(" | Raw touch: ");
      Serial.print(rawTouchX);
      Serial.print(",");
      Serial.print(rawTouchY);

      Serial.print(" | Mapped touch: ");
      Serial.print(touchX);
      Serial.print(",");
      Serial.print(touchY);
    }

    Serial.println();
  }
}

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
        strncpy(lastNmeaLine, nmeaLine, sizeof(lastNmeaLine));
        lastNmeaLine[sizeof(lastNmeaLine) - 1] = '\0';
        gnssSentences++;
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

void updateSDLogging()
{
  if (!sdOK)
  {
    return;
  }

  if (millis() - lastLogTime < LOG_INTERVAL_MS)
  {
    return;
  }

  lastLogTime = millis();
  appendLogRow();
}

void updateTouch()
{
  uint16_t x;
  uint16_t y;

  bool pressed = readMappedTouch(&x, &y);

  touchPressed = pressed;

  if (!pressed)
  {
    touchWasDown = false;
    return;
  }

  touchX = x;
  touchY = y;

  if (touchWasDown)
  {
    return;
  }

  touchWasDown = true;

  if (millis() - lastTouchAction < 250)
  {
    return;
  }

  lastTouchAction = millis();

  Serial.print("Touch raw: ");
  Serial.print(rawTouchX);
  Serial.print(", ");
  Serial.print(rawTouchY);

  Serial.print(" | mapped: ");
  Serial.print(touchX);
  Serial.print(", ");
  Serial.println(touchY);

  processTouch(touchX, touchY);
}

// ------------------------------------------------------------
// Touch
// ------------------------------------------------------------

bool readMappedTouch(uint16_t *xOut, uint16_t *yOut)
{
  uint16_t rx;
  uint16_t ry;

  if (!bigTft.getTouch(&rx, &ry))
  {
    return false;
  }

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
  if (y >= NAV_Y && y <= BIG_H - 1)
  {
    if (x >= NAV_BACK_X0 && x <= NAV_BACK_X1)
    {
      gentleBeep();
      previousMenu();
      return;
    }

    if (x >= NAV_HOME_X0 && x <= NAV_HOME_X1)
    {
      gentleBeep();
      goHome();
      return;
    }

    if (x >= NAV_NEXT_X0 && x <= NAV_NEXT_X1)
    {
      gentleBeep();
      nextMenu();
      return;
    }
  }

  if (currentMenu == MENU_HOME)
  {
    for (int i = 0; i < 6; i++)
    {
      int boxX = 14;
      int boxY = 65 + i * 52;
      int boxW = 292;
      int boxH = 38;

      if (x >= boxX && x <= boxX + boxW && y >= boxY && y <= boxY + boxH)
      {
        gentleBeep();
        currentMenu = (MenuPage)(i + 1);
        forceRedraw = true;
        return;
      }
    }
  }
}

// ------------------------------------------------------------
// Serial / boot button
// ------------------------------------------------------------

void handleSerialInput()
{
  while (Serial.available() > 0)
  {
    char c = Serial.read();

    if (c == '\n' || c == '\r' || c == ' ' || c == ',')
    {
      continue;
    }

    if (c == 'n' || c == 'N' || c == '+')
    {
      gentleBeep();
      nextMenu();
    }
    else if (c == 'p' || c == 'P' || c == '-')
    {
      gentleBeep();
      previousMenu();
    }
    else if (c == 'h' || c == 'H' || c == '0')
    {
      gentleBeep();
      goHome();
    }
    else if (c >= '1' && c <= '6')
    {
      gentleBeep();
      currentMenu = (MenuPage)(c - '0');
      forceRedraw = true;
    }
    else if (c == 'w' || c == 'W')
    {
      connectWiFi(true);
    }
    else if (c == 'u' || c == 'U')
    {
      uploadTelemetry();
    }
  }
}

void handleBootButton()
{
  bool pressed = digitalRead(BOOT_BUTTON_PIN) == LOW;

  if (pressed && millis() - lastBootPress > 400)
  {
    lastBootPress = millis();
    gentleBeep();
    nextMenu();
  }
}

// ------------------------------------------------------------
// Navigation
// ------------------------------------------------------------

void nextMenu()
{
  currentMenu = (MenuPage)(((int)currentMenu + 1) % MENU_COUNT);
  forceRedraw = true;

  Serial.print("Next menu: ");
  Serial.println((int)currentMenu);
}

void previousMenu()
{
  int newMenu = (int)currentMenu - 1;

  if (newMenu < 0)
  {
    newMenu = MENU_COUNT - 1;
  }

  currentMenu = (MenuPage)newMenu;
  forceRedraw = true;

  Serial.print("Previous menu: ");
  Serial.println((int)currentMenu);
}

void goHome()
{
  currentMenu = MENU_HOME;
  forceRedraw = true;

  Serial.println("Home menu");
}

// ------------------------------------------------------------
// GNSS helpers
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
  if (gps.satellites.isValid())
  {
    return gps.satellites.value();
  }

  return 0;
}

float gnssHdop()
{
  if (gps.hdop.isValid())
  {
    return gps.hdop.hdop();
  }

  return 99.99f;
}

const char *qualityText()
{
  if (!gnssReceiving())
  {
    return "NO DATA";
  }

  if (!gnssHasFix())
  {
    return "POOR";
  }

  if (gnssSatellites() >= 8 && gnssHdop() <= 2.0)
  {
    return "GOOD";
  }

  if (gnssSatellites() >= 4 && gnssHdop() <= 5.0)
  {
    return "OK";
  }

  return "POOR";
}

uint16_t qualityColour()
{
  const char *q = qualityText();

  if (strcmp(q, "GOOD") == 0)
  {
    return TFT_GREEN;
  }

  if (strcmp(q, "OK") == 0)
  {
    return TFT_YELLOW;
  }

  if (strcmp(q, "NO DATA") == 0)
  {
    return TFT_ORANGE;
  }

  return TFT_RED;
}

const char *qualityReason()
{
  if (!gnssReceiving())
  {
    return "No NMEA data";
  }

  if (!gnssHasFix())
  {
    return "No valid GNSS fix";
  }

  if (gnssSatellites() < 4)
  {
    return "Too few satellites";
  }

  if (gnssHdop() > 5.0)
  {
    return "HDOP too high";
  }

  if (gnssSatellites() < 8)
  {
    return "Moderate satellite count";
  }

  if (gnssHdop() > 2.0)
  {
    return "Moderate HDOP";
  }

  return "Signal healthy";
}

// ------------------------------------------------------------
// Thermal helpers
// ------------------------------------------------------------

const char *thermalStatus()
{
  if (!bmpOK)
  {
    return "NO BMP";
  }

  if (bmpTempC < 50.0)
  {
    return "OK";
  }

  if (bmpTempC < 60.0)
  {
    return "WARM";
  }

  return "HOT";
}

uint16_t thermalColour()
{
  if (!bmpOK)
  {
    return TFT_RED;
  }

  if (bmpTempC < 50.0)
  {
    return TFT_GREEN;
  }

  if (bmpTempC < 60.0)
  {
    return TFT_YELLOW;
  }

  return TFT_RED;
}

// ------------------------------------------------------------
// SD logging
// ------------------------------------------------------------

void createLogHeaderIfNeeded()
{
  if (!sdOK)
  {
    return;
  }

  bool exists = SD.exists(LOG_FILE);

  File file = SD.open(LOG_FILE, FILE_APPEND);

  if (!file)
  {
    Serial.println("Could not open log file");
    sdOK = false;
    strcpy(sdStatusText, "FILE FAIL");
    return;
  }

  if (!exists || file.size() == 0)
  {
    file.println("millis,utc_date,utc_time,fix,latitude,longitude,altitude_m,speed_kmh,satellites,hdop,quality,bmp_temp_c,bmp_pressure_hpa,acc_x,acc_y,acc_z,acc_mag,nmea_sentences,wifi,server,http_code,upload_count,server_command,screen_colour,distance_m");
  }

  file.close();
}

void appendLogRow()
{
  File file = SD.open(LOG_FILE, FILE_APPEND);

  if (!file)
  {
    Serial.println("Could not append to SD log");
    sdOK = false;
    strcpy(sdStatusText, "LOG FAIL");
    return;
  }

  file.print(millis());
  file.print(",");

  if (gps.date.isValid())
  {
    file.print(gps.date.year());
    file.print("-");
    if (gps.date.month() < 10) file.print("0");
    file.print(gps.date.month());
    file.print("-");
    if (gps.date.day() < 10) file.print("0");
    file.print(gps.date.day());
  }

  file.print(",");

  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) file.print("0");
    file.print(gps.time.hour());
    file.print(":");
    if (gps.time.minute() < 10) file.print("0");
    file.print(gps.time.minute());
    file.print(":");
    if (gps.time.second() < 10) file.print("0");
    file.print(gps.time.second());
  }

  file.print(",");
  file.print(gnssHasFix() ? "YES" : "NO");
  file.print(",");

  printFloatOrBlank(file, gps.location.isValid(), gps.location.lat(), 6);
  file.print(",");

  printFloatOrBlank(file, gps.location.isValid(), gps.location.lng(), 6);
  file.print(",");

  printFloatOrBlank(file, gps.altitude.isValid(), gps.altitude.meters(), 1);
  file.print(",");

  printFloatOrBlank(file, gps.speed.isValid(), gps.speed.kmph(), 1);
  file.print(",");

  file.print(gnssSatellites());
  file.print(",");

  printFloatOrBlank(file, gps.hdop.isValid(), gnssHdop(), 2);
  file.print(",");

  file.print(qualityText());
  file.print(",");

  printFloatOrBlank(file, bmpOK, bmpTempC, 1);
  file.print(",");

  printFloatOrBlank(file, bmpOK, bmpPressureHpa, 1);
  file.print(",");

  printFloatOrBlank(file, qmiOK, accX, 3);
  file.print(",");

  printFloatOrBlank(file, qmiOK, accY, 3);
  file.print(",");

  printFloatOrBlank(file, qmiOK, accZ, 3);
  file.print(",");

  printFloatOrBlank(file, qmiOK, accMagnitude, 3);
  file.print(",");

  file.print(gnssSentences);
  file.print(",");

  file.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");
  file.print(",");

  file.print(uploadStatusText);
  file.print(",");

  file.print(lastHttpCode);
  file.print(",");

  file.print(uploadCount);
  file.print(",");

  file.print(serverCommandText);
  file.print(",");

  file.print(serverScreenColour);
  file.print(",");

  if (serverDistanceM >= 0)
  {
    file.print(serverDistanceM, 1);
  }

  file.println();

  file.close();

  logRows++;

  Serial.print("Logged row ");
  Serial.print(logRows);
  Serial.print(" to ");
  Serial.println(LOG_FILE);
}

void printFloatOrBlank(File &file, bool valid, double value, int decimals)
{
  if (valid)
  {
    file.print(value, decimals);
  }
}

// ------------------------------------------------------------
// Display drawing
// ------------------------------------------------------------

void drawDisplay()
{
  bigTft.fillScreen(TFT_BLACK);

  switch (currentMenu)
  {
    case MENU_HOME:
      drawHomeMenu();
      break;

    case MENU_LIVE_GNSS:
      drawLiveGNSSMenu();
      break;

    case MENU_SATELLITES:
      drawSatellitesMenu();
      break;

    case MENU_SIGNAL_QUALITY:
      drawSignalQualityMenu();
      break;

    case MENU_FSPL:
      drawFSPLMenu();
      break;

    case MENU_CAMPUS_SURVEY:
      drawCampusSurveyMenu();
      break;

    case MENU_SYSTEM:
      drawSystemMenu();
      break;

    default:
      drawHomeMenu();
      break;
  }
}

void updateDisplayValues()
{
  switch (currentMenu)
  {
    case MENU_HOME:
      updateHomeValues();
      break;

    case MENU_LIVE_GNSS:
      updateLiveGNSSValues();
      break;

    case MENU_SATELLITES:
      updateSatelliteValues();
      break;

    case MENU_SIGNAL_QUALITY:
      updateSignalQualityValues();
      break;

    case MENU_CAMPUS_SURVEY:
      updateCampusSurveyValues();
      break;

    case MENU_SYSTEM:
      updateSystemValues();
      break;

    default:
      break;
  }
}

void drawHeader(const char *title)
{
  bigTft.fillRect(0, 0, BIG_W, 52, TFT_NAVY);

  bigTft.setTextColor(TFT_WHITE, TFT_NAVY);
  bigTft.setTextSize(2);
  bigTft.setCursor(10, 17);
  bigTft.print(title);

  drawNavBar();
}

void drawNavBar()
{
  bigTft.fillRect(0, NAV_Y, BIG_W, NAV_H, TFT_DARKGREY);

  bigTft.drawFastVLine(NAV_BACK_X1, NAV_Y, NAV_H, TFT_BLACK);
  bigTft.drawFastVLine(NAV_HOME_X1, NAV_Y, NAV_H, TFT_BLACK);

  drawBackIcon(53, 445, TFT_WHITE);
  drawHomeIcon(160, 445, TFT_WHITE);
  drawNextIcon(267, 445, TFT_WHITE);
}

void drawBackIcon(int cx, int cy, uint16_t colour)
{
  bigTft.fillTriangle(cx - 16, cy, cx + 5, cy - 14, cx + 5, cy + 14, colour);
  bigTft.fillRect(cx + 4, cy - 6, 20, 12, colour);
}

void drawNextIcon(int cx, int cy, uint16_t colour)
{
  bigTft.fillTriangle(cx + 16, cy, cx - 5, cy - 14, cx - 5, cy + 14, colour);
  bigTft.fillRect(cx - 24, cy - 6, 20, 12, colour);
}

void drawHomeIcon(int cx, int cy, uint16_t colour)
{
  bigTft.fillTriangle(cx - 18, cy - 2, cx, cy - 20, cx + 18, cy - 2, colour);
  bigTft.fillRect(cx - 13, cy - 2, 26, 22, colour);
  bigTft.fillRect(cx - 4, cy + 7, 8, 13, TFT_DARKGREY);
}

// ------------------------------------------------------------
// Home menu
// ------------------------------------------------------------

void drawHomeMenu()
{
  drawHeader("GNSS FIELD TOOL");

  const char *items[] = {
    "Live GNSS",
    "Satellite Data",
    "Signal Quality",
    "FSPL Calculator",
    "Campus Survey",
    "System Status"
  };

  bigTft.setTextSize(2);

  for (int i = 0; i < 6; i++)
  {
    int y = 65 + i * 52;

    bigTft.drawRoundRect(14, y, 292, 38, 8, TFT_DARKGREY);

    bigTft.setTextColor(TFT_CYAN, TFT_BLACK);
    bigTft.setCursor(26, y + 11);
    bigTft.print(i + 1);
    bigTft.print(". ");

    bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
    bigTft.print(items[i]);
  }

  updateHomeValues();
}

void updateHomeValues()
{
  bigTft.fillRect(18, 365, 285, 35, TFT_BLACK);

  bigTft.setTextSize(1);

  bigTft.setTextColor(qualityColour(), TFT_BLACK);
  bigTft.setCursor(18, 365);
  bigTft.print("GNSS: ");
  bigTft.print(qualityText());
  bigTft.print(" | Sats: ");
  bigTft.print(gnssSatellites());

  bigTft.setTextColor(sdOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(" | SD: ");
  bigTft.print(sdStatusText);

  bigTft.setTextColor(WiFi.status() == WL_CONNECTED ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.setCursor(18, 382);
  bigTft.print("WiFi: ");
  bigTft.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");

  bigTft.setTextColor(serverOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(" | Web: ");
  bigTft.print(uploadStatusText);

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.print(" | Up:");
  bigTft.print(uploadCount);
}

// ------------------------------------------------------------
// Live GNSS
// ------------------------------------------------------------

void drawLiveGNSSMenu()
{
  drawHeader("LIVE GNSS");

  bigTft.setTextSize(2);
  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(20, 70);
  bigTft.print("Data:");

  bigTft.setCursor(20, 108);
  bigTft.print("Fix:");

  bigTft.setCursor(20, 146);
  bigTft.print("Lat:");

  bigTft.setCursor(20, 184);
  bigTft.print("Lon:");

  bigTft.setCursor(20, 222);
  bigTft.print("Alt:");

  bigTft.setCursor(20, 260);
  bigTft.print("Sats:");

  bigTft.setCursor(20, 298);
  bigTft.print("HDOP:");

  bigTft.setCursor(20, 336);
  bigTft.print("Speed:");

  updateLiveGNSSValues();
}

void updateLiveGNSSValues()
{
  bigTft.fillRect(110, 65, 200, 320, TFT_BLACK);

  bigTft.setTextSize(2);

  bigTft.setCursor(110, 70);
  bigTft.setTextColor(gnssReceiving() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(gnssReceiving() ? "RX" : "WAIT");

  bigTft.setCursor(110, 108);
  bigTft.setTextColor(gnssHasFix() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(gnssHasFix() ? "VALID" : "NO FIX");

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(110, 146);
  if (gps.location.isValid())
  {
    bigTft.print(gps.location.lat(), 6);
  }
  else
  {
    bigTft.print("--");
  }

  bigTft.setCursor(110, 184);
  if (gps.location.isValid())
  {
    bigTft.print(gps.location.lng(), 6);
  }
  else
  {
    bigTft.print("--");
  }

  bigTft.setCursor(110, 222);
  if (gps.altitude.isValid())
  {
    bigTft.print(gps.altitude.meters(), 1);
    bigTft.print("m");
  }
  else
  {
    bigTft.print("--");
  }

  bigTft.setCursor(110, 260);
  bigTft.print(gnssSatellites());

  bigTft.setCursor(110, 298);
  if (gps.hdop.isValid())
  {
    bigTft.print(gnssHdop(), 2);
  }
  else
  {
    bigTft.print("--");
  }

  bigTft.setCursor(110, 336);
  if (gps.speed.isValid())
  {
    bigTft.print(gps.speed.kmph(), 1);
    bigTft.print("km/h");
  }
  else
  {
    bigTft.print("--");
  }
}

// ------------------------------------------------------------
// Satellite data
// ------------------------------------------------------------

void drawSatellitesMenu()
{
  drawHeader("SATELLITE DATA");

  bigTft.setTextSize(2);
  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(20, 80);
  bigTft.print("Satellites:");

  bigTft.setCursor(20, 130);
  bigTft.print("HDOP:");

  bigTft.setCursor(20, 180);
  bigTft.print("Chars:");

  bigTft.setCursor(20, 230);
  bigTft.print("Sentences:");

  bigTft.setTextSize(1);
  bigTft.setTextColor(TFT_CYAN, TFT_BLACK);
  bigTft.setCursor(20, 300);
  bigTft.print("Last NMEA:");

  updateSatelliteValues();
}

void updateSatelliteValues()
{
  bigTft.fillRect(170, 75, 135, 210, TFT_BLACK);

  bigTft.setTextSize(2);
  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(170, 80);
  bigTft.print(gnssSatellites());

  bigTft.setCursor(170, 130);
  if (gps.hdop.isValid())
  {
    bigTft.print(gnssHdop(), 2);
  }
  else
  {
    bigTft.print("--");
  }

  bigTft.setCursor(170, 180);
  bigTft.print(gnssChars);

  bigTft.setCursor(170, 230);
  bigTft.print(gnssSentences);

  bigTft.fillRect(20, 325, 280, 75, TFT_BLACK);
  bigTft.setTextSize(1);
  bigTft.setTextColor(TFT_YELLOW, TFT_BLACK);
  bigTft.setCursor(20, 325);
  bigTft.print(lastNmeaLine);
}

// ------------------------------------------------------------
// Signal quality
// ------------------------------------------------------------

void drawSignalQualityMenu()
{
  drawHeader("SIGNAL QUALITY");

  bigTft.setTextSize(2);
  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(20, 80);
  bigTft.print("Quality:");

  bigTft.setCursor(20, 170);
  bigTft.print("Reason:");

  bigTft.setCursor(20, 245);
  bigTft.print("Sats:");

  bigTft.setCursor(20, 295);
  bigTft.print("HDOP:");

  updateSignalQualityValues();
}

void updateSignalQualityValues()
{
  bigTft.fillRect(20, 112, 285, 245, TFT_BLACK);

  bigTft.setTextSize(3);
  bigTft.setTextColor(qualityColour(), TFT_BLACK);
  bigTft.setCursor(20, 115);
  bigTft.print(qualityText());

  bigTft.setTextSize(2);
  bigTft.setTextColor(TFT_YELLOW, TFT_BLACK);
  bigTft.setCursor(20, 200);
  bigTft.print(qualityReason());

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(120, 245);
  bigTft.print(gnssSatellites());

  bigTft.setCursor(120, 295);
  if (gps.hdop.isValid())
  {
    bigTft.print(gnssHdop(), 2);
  }
  else
  {
    bigTft.print("--");
  }
}

// ------------------------------------------------------------
// FSPL
// ------------------------------------------------------------

void drawFSPLMenu()
{
  drawHeader("FSPL CALC");

  float gpsL1 = 1575.42;
  float gpsL5 = 1176.45;

  float fsplL1_100m = calculateFSPL(0.1, gpsL1);
  float fsplL1_1km = calculateFSPL(1.0, gpsL1);
  float fsplL5_1km = calculateFSPL(1.0, gpsL5);

  bigTft.setTextSize(2);
  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(20, 80);
  bigTft.print("GPS L1:");

  bigTft.setCursor(20, 108);
  bigTft.print(gpsL1, 2);
  bigTft.print(" MHz");

  bigTft.setCursor(20, 150);
  bigTft.print("GPS L5:");

  bigTft.setCursor(20, 178);
  bigTft.print(gpsL5, 2);
  bigTft.print(" MHz");

  bigTft.setTextColor(TFT_CYAN, TFT_BLACK);

  bigTft.setCursor(20, 235);
  bigTft.print("L1 @100m:");

  bigTft.setCursor(20, 263);
  bigTft.print(fsplL1_100m, 1);
  bigTft.print(" dB");

  bigTft.setCursor(20, 305);
  bigTft.print("L1 @1km:");

  bigTft.setCursor(20, 333);
  bigTft.print(fsplL1_1km, 1);
  bigTft.print(" dB");

  bigTft.setCursor(20, 365);
  bigTft.print("L5 @1km:");

  bigTft.setCursor(20, 393);
  bigTft.print(fsplL5_1km, 1);
  bigTft.print(" dB");
}

// ------------------------------------------------------------
// Campus survey / SD + Web upload
// ------------------------------------------------------------

void drawCampusSurveyMenu()
{
  drawHeader("CAMPUS SURVEY");

  bigTft.setTextSize(2);
  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(20, 70);
  bigTft.print("SD:");

  bigTft.setCursor(20, 110);
  bigTft.print("Rows:");

  bigTft.setCursor(20, 150);
  bigTft.print("Fix:");

  bigTft.setCursor(20, 190);
  bigTft.print("WiFi:");

  bigTft.setCursor(20, 230);
  bigTft.print("Web:");

  bigTft.setCursor(20, 270);
  bigTft.print("HTTP:");

  bigTft.setCursor(20, 310);
  bigTft.print("Cmd:");

  bigTft.setTextSize(1);
  bigTft.setTextColor(TFT_CYAN, TFT_BLACK);
  bigTft.setCursor(20, 360);
  bigTft.print("CSV to SD + JSON upload to FastAPI.");

  bigTft.setCursor(20, 380);
  bigTft.print(SERVER_URL);

  updateCampusSurveyValues();
}

void updateCampusSurveyValues()
{
  bigTft.fillRect(100, 65, 210, 285, TFT_BLACK);

  bigTft.setTextSize(2);

  bigTft.setCursor(100, 70);
  bigTft.setTextColor(sdOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(sdStatusText);

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(100, 110);
  bigTft.print(logRows);

  bigTft.setCursor(100, 150);
  bigTft.setTextColor(gnssHasFix() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(gnssHasFix() ? "YES" : "NO");

  bigTft.setCursor(100, 190);
  bigTft.setTextColor(WiFi.status() == WL_CONNECTED ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setCursor(100, 210);
  if (WiFi.status() == WL_CONNECTED)
  {
    bigTft.print(WiFi.localIP());
  }
  else
  {
    bigTft.print("-");
  }

  bigTft.setCursor(100, 230);
  bigTft.setTextColor(serverOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(uploadStatusText);

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setCursor(100, 270);
  bigTft.print(lastHttpCode);

  bigTft.setCursor(100, 310);
  bigTft.print(serverCommandText);

  bigTft.setCursor(100, 330);
  bigTft.print("Up ");
  bigTft.print(uploadCount);
  bigTft.print(" Fail ");
  bigTft.print(uploadFailCount);
}

// ------------------------------------------------------------
// System status
// ------------------------------------------------------------

void drawSystemMenu()
{
  drawHeader("SYSTEM STATUS");

  bigTft.setTextSize(1);
  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);

  bigTft.setCursor(20, 65);
  bigTft.print("BMP280");

  bigTft.setCursor(20, 145);
  bigTft.print("QMI8658C");

  bigTft.setCursor(20, 250);
  bigTft.print("GNSS / SD / TOUCH / WEB");

  updateSystemValues();
}

void updateSystemValues()
{
  bigTft.fillRect(20, 82, 285, 320, TFT_BLACK);

  bigTft.setTextSize(1);

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setCursor(20, 85);
  bigTft.print("BMP: ");

  bigTft.setTextColor(bmpOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(bmpOK ? "OK" : "NO");

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setCursor(20, 105);
  bigTft.print("Temp: ");

  bigTft.setTextColor(thermalColour(), TFT_BLACK);
  bigTft.print(bmpTempC, 1);
  bigTft.print(" C ");
  bigTft.print(thermalStatus());

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setCursor(20, 125);
  bigTft.print("Pressure: ");
  bigTft.print(bmpPressureHpa, 1);
  bigTft.print(" hPa");

  bigTft.setCursor(20, 165);
  bigTft.print("QMI: ");

  bigTft.setTextColor(qmiOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(qmiOK ? "OK" : "NO");

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setCursor(20, 185);
  bigTft.print("AX:");
  bigTft.print(accX, 2);

  bigTft.setCursor(110, 185);
  bigTft.print("AY:");
  bigTft.print(accY, 2);

  bigTft.setCursor(200, 185);
  bigTft.print("AZ:");
  bigTft.print(accZ, 2);

  bigTft.setCursor(20, 230);
  bigTft.print("GNSS chars: ");
  bigTft.print(gnssChars);

  bigTft.setCursor(20, 250);
  bigTft.print("Sats: ");
  bigTft.print(gnssSatellites());
  bigTft.print(" Fix: ");
  bigTft.print(gnssHasFix() ? "YES" : "NO");

  bigTft.setCursor(20, 270);
  bigTft.print("SD: ");

  bigTft.setTextColor(sdOK ? TFT_GREEN : TFT_RED, TFT_BLACK);
  bigTft.print(sdStatusText);

  bigTft.setTextColor(TFT_WHITE, TFT_BLACK);
  bigTft.setCursor(20, 290);
  bigTft.print("Logs: ");
  bigTft.print(logRows);

  bigTft.setCursor(20, 310);
  bigTft.print("Raw touch: ");
  bigTft.print(rawTouchX);
  bigTft.print(",");
  bigTft.print(rawTouchY);

  bigTft.setCursor(20, 330);
  bigTft.print("Mapped: ");
  bigTft.print(touchX);
  bigTft.print(",");
  bigTft.print(touchY);

  bigTft.setCursor(20, 350);
  bigTft.print("WiFi: ");
  bigTft.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");

  bigTft.print(" HTTP:");
  bigTft.print(lastHttpCode);

  bigTft.setCursor(20, 370);
  bigTft.print("Server: ");
  bigTft.print(uploadStatusText);
  bigTft.print(" Up:");
  bigTft.print(uploadCount);

  bigTft.setCursor(20, 390);
  bigTft.print("Cmd: ");
  bigTft.print(serverCommandText);
}

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------

float calculateFSPL(float distanceKm, float frequencyMHz)
{
  return 32.44f + 20.0f * log10(distanceKm) + 20.0f * log10(frequencyMHz);
}