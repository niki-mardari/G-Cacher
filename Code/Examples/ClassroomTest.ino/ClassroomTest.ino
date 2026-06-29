/*
  Tenstar ESP32-S3 + LC29H GNSS Display Test
  ------------------------------------------

  Shows:
    - Raw NMEA in Serial Monitor
    - Latitude
    - Longitude
    - Speed in km/h
    - UTC time
    - Satellites
    - HDOP
    - Google Maps link in Serial Monitor

  Wiring:
    LC29H TX  -> ESP32 GPIO17
    LC29H RX  -> ESP32 GPIO18
    LC29H GND -> ESP32 GND

  Serial Monitor:
    115200 baud

  Libraries:
    - TinyGPSPlus
    - Adafruit GFX Library
    - Adafruit ST7735 and ST7789 Library
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <TinyGPSPlus.h>

// -------------------- GNSS UART PINS --------------------

#define GNSS_RX_PIN 17
#define GNSS_TX_PIN 18
#define GNSS_BAUD   115200

// -------------------- TFT DISPLAY PINS --------------------

#define TFT_CS         7
#define TFT_DC         39
#define TFT_RST        40
#define TFT_BACKLIGHT  45

// -------------------- SPI PINS FOR TFT --------------------

#define SPI_SCK   36
#define SPI_MISO  37
#define SPI_MOSI  35

// -------------------- OBJECTS --------------------

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
TinyGPSPlus gps;

// This stores one full NMEA sentence for printing
String nmeaSentence = "";

// Timing
unsigned long lastScreenUpdate = 0;
unsigned long lastSerialSummary = 0;

const unsigned long SCREEN_UPDATE_MS = 1000;
const unsigned long SERIAL_SUMMARY_MS = 3000;

// -------------------- FUNCTION DECLARATIONS --------------------

void startDisplay();
void drawStaticScreen();
void updateDisplay();
void printSerialSummary();
void printGoogleMapsLink();

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("========================================");
  Serial.println("Tenstar ESP32-S3 + LC29H GNSS Display");
  Serial.println("========================================");
  Serial.println("GNSS baud: 115200");
  Serial.println("GNSS RX pin: GPIO17");
  Serial.println("GNSS TX pin: GPIO18");
  Serial.println();

  // Start display SPI
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, TFT_CS);

  startDisplay();
  drawStaticScreen();

  // Start GNSS UART
  Serial1.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);

  nmeaSentence.reserve(120);

  Serial.println("Waiting for NMEA data...");
  Serial.println();
}

void loop() {
  while (Serial1.available()) {
    char c = Serial1.read();

    // Feed every character into TinyGPSPlus
    gps.encode(c);

    // Also build a full raw NMEA sentence for Serial Monitor
    if (c == '\n') {
      nmeaSentence.trim();

      if (nmeaSentence.length() > 0) {
        Serial.println(nmeaSentence);
      }

      nmeaSentence = "";
    } else {
      nmeaSentence += c;
    }
  }

  if (millis() - lastScreenUpdate >= SCREEN_UPDATE_MS) {
    lastScreenUpdate = millis();
    updateDisplay();
  }

  if (millis() - lastSerialSummary >= SERIAL_SUMMARY_MS) {
    lastSerialSummary = millis();
    printSerialSummary();
  }
}

// -------------------- DISPLAY SETUP --------------------

void startDisplay() {
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  tft.init(135, 240);
  tft.setRotation(3);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  Serial.println("Display started");
}

// -------------------- STATIC SCREEN --------------------

void drawStaticScreen() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(0, 0);
  tft.print("LC29H GNSS");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(0, 28);
  tft.print("Fix:");

  tft.setCursor(0, 45);
  tft.print("Lat:");

  tft.setCursor(0, 62);
  tft.print("Lon:");

  tft.setCursor(0, 79);
  tft.print("Speed:");

  tft.setCursor(0, 96);
  tft.print("Time:");

  tft.setCursor(0, 113);
  tft.print("Sat/HDOP:");
}

// -------------------- DISPLAY UPDATE --------------------

void updateDisplay() {
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  // Clear value areas only
  tft.fillRect(70, 28, 165, 105, ST77XX_BLACK);

  // Fix status
  tft.setCursor(70, 28);

  if (gps.location.isValid()) {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
    tft.print("VALID");
  } else {
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.print("NO FIX");
  }

  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  // Latitude
  tft.setCursor(70, 45);
  if (gps.location.isValid()) {
    tft.print(gps.location.lat(), 6);
  } else {
    tft.print("waiting...");
  }

  // Longitude
  tft.setCursor(70, 62);
  if (gps.location.isValid()) {
    tft.print(gps.location.lng(), 6);
  } else {
    tft.print("waiting...");
  }

  // Speed
  tft.setCursor(70, 79);
  if (gps.speed.isValid()) {
    tft.print(gps.speed.kmph(), 1);
    tft.print(" km/h");
  } else {
    tft.print("waiting...");
  }

  // Time
  tft.setCursor(70, 96);
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) tft.print("0");
    tft.print(gps.time.hour());
    tft.print(":");

    if (gps.time.minute() < 10) tft.print("0");
    tft.print(gps.time.minute());
    tft.print(":");

    if (gps.time.second() < 10) tft.print("0");
    tft.print(gps.time.second());

    tft.print(" UTC");
  } else {
    tft.print("waiting...");
  }

  // Satellites and HDOP
  tft.setCursor(70, 113);

  if (gps.satellites.isValid()) {
    tft.print(gps.satellites.value());
  } else {
    tft.print("-");
  }

  tft.print(" / ");

  if (gps.hdop.isValid()) {
    tft.print(gps.hdop.hdop(), 1);
  } else {
    tft.print("-");
  }
}

// -------------------- SERIAL SUMMARY --------------------

void printSerialSummary() {
  Serial.println();
  Serial.println("---------- Parsed GNSS Data ----------");

  if (gps.location.isValid()) {
    Serial.print("Latitude:  ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);

    printGoogleMapsLink();
  } else {
    Serial.println("Location: waiting for valid fix...");
  }

  if (gps.speed.isValid()) {
    Serial.print("Speed: ");
    Serial.print(gps.speed.kmph(), 2);
    Serial.println(" km/h");
  } else {
    Serial.println("Speed: waiting...");
  }

  if (gps.time.isValid()) {
    Serial.print("UTC Time: ");

    if (gps.time.hour() < 10) Serial.print("0");
    Serial.print(gps.time.hour());
    Serial.print(":");

    if (gps.time.minute() < 10) Serial.print("0");
    Serial.print(gps.time.minute());
    Serial.print(":");

    if (gps.time.second() < 10) Serial.print("0");
    Serial.println(gps.time.second());
  } else {
    Serial.println("Time: waiting...");
  }

  if (gps.satellites.isValid()) {
    Serial.print("Satellites: ");
    Serial.println(gps.satellites.value());
  } else {
    Serial.println("Satellites: waiting...");
  }

  if (gps.hdop.isValid()) {
    Serial.print("HDOP: ");
    Serial.println(gps.hdop.hdop(), 2);
  } else {
    Serial.println("HDOP: waiting...");
  }

  Serial.println("--------------------------------------");
  Serial.println();
}

// -------------------- GOOGLE MAPS LINK --------------------

void printGoogleMapsLink() {
  Serial.print("Google Maps: ");
  Serial.print("https://www.google.com/maps/search/?api=1&query=");
  Serial.print(gps.location.lat(), 6);
  Serial.print(",");
  Serial.println(gps.location.lng(), 6);
}