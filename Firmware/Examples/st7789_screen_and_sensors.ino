/*
  TENSTAR TS-ESP32-S3 Simple Sensor Display
  ----------------------------------------

  Board:
    TENSTAR TS-ESP32-S3 Development Board
    ESP32-S3 FH4R2
    Onboard 1.14" ST7789 TFT
    Onboard BMP280 temperature/pressure sensor
    Onboard QMI8658C accelerometer/gyroscope

  Arduino IDE board:
    Tools → Board → ESP32S3 Dev Module

  Useful board settings:
    USB CDC On Boot: Enabled
    Flash Size: 4MB
    PSRAM: Enabled / QSPI PSRAM

  Libraries to install:
    1. Adafruit GFX Library
    2. Adafruit ST7735 and ST7789 Library
    3. Adafruit BMP280 Library
    4. SensorLib

  Important QMI8658 note:
    Do NOT use:
      #include <SensorQMI8658.hpp>

    That is the old/deprecated driver.

    Use:
      #include "ImuDrv.hpp"

  What this sketch does:
    - Starts the TFT display
    - Starts I2C sensors
    - Reads BMP280 temperature
    - Reads QMI8658C acceleration values
    - Displays Temperature, AX, AY, AZ
    - Updates only once per second to reduce flicker
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_BMP280.h>

#include "ImuDrv.hpp"   // Newer SensorLib QMI8658 driver
// Donwload here: https://github.com/lewisxhe/SensorLib/tree/master

// -------------------- TFT DISPLAY PINS --------------------

#define TFT_CS         7
#define TFT_DC         39
#define TFT_RST        40
#define TFT_BACKLIGHT  45

// -------------------- SPI PINS FOR TFT --------------------

#define SPI_SCK   36
#define SPI_MISO  37
#define SPI_MOSI  35

// -------------------- I2C PINS FOR SENSORS --------------------

#define I2C_SDA  42
#define I2C_SCL  41

// -------------------- SENSOR ADDRESSES --------------------

#define BMP_ADDR_1  0x77
#define BMP_ADDR_2  0x76

#define QMI_ADDR_1  0x6B
#define QMI_ADDR_2  0x6A

// -------------------- OBJECTS --------------------

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_BMP280 bmp;
SensorQMI8658 imu;

// These hold QMI8658 acceleration and gyro data
AccelerometerData accel;
GyroscopeData gyro;

// -------------------- STATUS FLAGS --------------------

bool bmpOK = false;
bool imuOK = false;

// -------------------- DISPLAY UPDATE TIMING --------------------

// Screen refreshes once per second.
// Increase to 2000 for slower updates.
// Decrease to 500 for faster updates.
const unsigned long SCREEN_UPDATE_MS = 1000;
unsigned long lastScreenUpdate = 0;

// -------------------- SENSOR VALUES --------------------

float temperatureC = 0.0;
float ax = 0.0;
float ay = 0.0;
float az = 0.0;

// -------------------- FUNCTION DECLARATIONS --------------------

void startDisplay();
void startBMP280();
void startQMI8658();
void drawLabelsOnce();
void readSensors();
void updateDisplayValues();

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("TENSTAR ESP32-S3 Simple Sensor Display");
  Serial.println("--------------------------------------");

  // Start SPI for the TFT display
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, TFT_CS);

  // Start I2C for BMP280 and QMI8658C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  startDisplay();

  tft.setCursor(0, 0);
  tft.println("Starting...");

  startBMP280();
  startQMI8658();

  // Draw fixed labels once.
  // This helps prevent flicker because labels are not redrawn constantly.
  drawLabelsOnce();

  Serial.println("Setup finished");
}

void loop() {
  // Read sensors regularly
  readSensors();

  // Only update the screen every 1 second
  if (millis() - lastScreenUpdate >= SCREEN_UPDATE_MS) {
    lastScreenUpdate = millis();
    updateDisplayValues();
  }

  delay(10);
}

// -------------------- DISPLAY SETUP --------------------

void startDisplay() {
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  tft.init(135, 240);
  tft.setRotation(3);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  Serial.println("Display started");
}

// -------------------- BMP280 SETUP --------------------

void startBMP280() {
  Serial.println("Starting BMP280...");

  if (bmp.begin(BMP_ADDR_1)) {
    bmpOK = true;
    Serial.println("BMP280 found at 0x77");
    return;
  }

  if (bmp.begin(BMP_ADDR_2)) {
    bmpOK = true;
    Serial.println("BMP280 found at 0x76");
    return;
  }

  bmpOK = false;
  Serial.println("BMP280 not found");
}

// -------------------- QMI8658 SETUP --------------------

void startQMI8658() {
  Serial.println("Starting QMI8658C...");

  if (imu.begin(Wire, QMI_ADDR_1, I2C_SDA, I2C_SCL)) {
    imuOK = true;
    Serial.println("QMI8658C found at 0x6B");
  } else if (imu.begin(Wire, QMI_ADDR_2, I2C_SDA, I2C_SCL)) {
    imuOK = true;
    Serial.println("QMI8658C found at 0x6A");
  } else {
    imuOK = false;
    Serial.println("QMI8658C not found");
    return;
  }

  // Configure accelerometer.
  // FS_8G means the accelerometer can measure up to +/- 8g.
  imu.configAccel(
    AccelFullScaleRange::FS_8G,
    1000.0f,
    SensorQMI8658::LpfMode::MODE_0
  );

  // Configure gyroscope.
  // We configure it even though this display only shows acceleration.
  imu.configGyro(
    GyroFullScaleRange::FS_1000_DPS,
    1000.0f,
    SensorQMI8658::LpfMode::MODE_0
  );

  imu.enableAccel();
  imu.enableGyro();

  Serial.println("QMI8658C started");
}

// -------------------- STATIC DISPLAY LAYOUT --------------------

void drawLabelsOnce() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);

  tft.setCursor(0, 0);
  tft.print("T:");

  tft.setCursor(0, 35);
  tft.print("AX:");

  tft.setCursor(0, 70);
  tft.print("AY:");

  tft.setCursor(0, 105);
  tft.print("AZ:");
}

// -------------------- SENSOR READING --------------------

void readSensors() {
  if (bmpOK) {
    temperatureC = bmp.readTemperature();
  }

  if (imuOK) {
    if (imu.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::ACCEL))) {
      imu.readAccel(accel);
      imu.readGyro(gyro);

      ax = accel.mps2.x;
      ay = accel.mps2.y;
      az = accel.mps2.z;
    }
  }
}

// -------------------- DISPLAY VALUE UPDATE --------------------

void updateDisplayValues() {
  /*
    Flicker reduction:
    Instead of clearing the whole screen, we only clear the number areas.

    This is why we use fillRect() beside each label.
  */

  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  // Temperature value area
  tft.fillRect(65, 0, 170, 30, ST77XX_BLACK);
  tft.setCursor(65, 0);

  if (bmpOK) {
    tft.print(temperatureC, 1);
    tft.print("C");
  } else {
    tft.print("ERR");
  }

  // AX value area
  tft.fillRect(65, 35, 170, 30, ST77XX_BLACK);
  tft.setCursor(65, 35);

  if (imuOK) {
    tft.print(ax, 1);
  } else {
    tft.print("ERR");
  }

  // AY value area
  tft.fillRect(65, 70, 170, 30, ST77XX_BLACK);
  tft.setCursor(65, 70);

  if (imuOK) {
    tft.print(ay, 1);
  } else {
    tft.print("ERR");
  }

  // AZ value area
  tft.fillRect(65, 105, 170, 30, ST77XX_BLACK);
  tft.setCursor(65, 105);

  if (imuOK) {
    tft.print(az, 1);
  } else {
    tft.print("ERR");
  }

  // Also print to Serial Monitor for debugging
  Serial.print("Temp: ");
  Serial.print(temperatureC, 1);
  Serial.print(" C | AX: ");
  Serial.print(ax, 1);
  Serial.print(" | AY: ");
  Serial.print(ay, 1);
  Serial.print(" | AZ: ");
  Serial.println(az, 1);
}
