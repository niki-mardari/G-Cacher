/*
  TENSTAR ESP32-S3 + LC29H GNSS / GPS Test
  ----------------------------------------

  Board:
    TENSTAR TS-ESP32-S3 Development Board

  GNSS Receiver:
    Waveshare / Quectel LC29H

  Arduino IDE board:
    Tools → Board → ESP32S3 Dev Module

  Library to install:
    TinyGPSPlus

  Wiring:
    LC29H TX  -> ESP32-S3 GPIO17  // ESP32 RX
    LC29H RX  -> ESP32-S3 GPIO18  // ESP32 TX
    LC29H GND -> ESP32-S3 GND

  Important:
    TX connects to RX.
    RX connects to TX.

  What this sketch does:
    1. Reads raw NMEA text from the LC29H.
    2. Shows the raw NMEA sentence.
    3. Explains the sentence type.
    4. Uses TinyGPSPlus to extract useful GPS data.
*/

#include <Arduino.h>
#include <TinyGPSPlus.h>

// -------------------- LC29H UART PINS --------------------

// LC29H TX goes to ESP32 RX
#define GNSS_RX_PIN 17

// LC29H RX goes to ESP32 TX
#define GNSS_TX_PIN 18

// LC29H commonly uses 115200 baud
#define GNSS_BAUD 115200

// Use UART1 on the ESP32-S3
HardwareSerial GNSS(1);

// TinyGPSPlus parses NMEA sentences for us
TinyGPSPlus gps;

// Used to print parsed data every second
unsigned long lastPrintTime = 0;

// Used to collect one full NMEA sentence
String nmeaSentence = "";

// -------------------- FUNCTION DECLARATIONS --------------------

void readGNSS();
void printGPSInfo();
void explainNMEASentence(String sentence);

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("====================================");
  Serial.println("TENSTAR ESP32-S3 + LC29H GNSS Test");
  Serial.println("====================================");
  Serial.println("Reading NMEA data from LC29H...");
  Serial.println();

  /*
    Start the GNSS serial port.

    Format:
      GNSS.begin(baud, serial_config, rx_pin, tx_pin);

    rx_pin = GPIO17 because ESP32 receives LC29H TX data there.
    tx_pin = GPIO18 because ESP32 sends data to LC29H RX there.
  */
  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
}

void loop() {
  // Read all available GNSS characters
  readGNSS();

  // Print parsed GPS information once per second
  if (millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();
    printGPSInfo();
  }
}

// -------------------- READ RAW GNSS DATA --------------------

void readGNSS() {
  while (GNSS.available()) {
    char c = GNSS.read();

    // Send every character into TinyGPSPlus
    gps.encode(c);

    /*
      Build a full NMEA sentence.

      NMEA sentences usually start with '$'
      and end with a new line.
    */
    if (c == '$') {
      nmeaSentence = "";
    }

    nmeaSentence += c;

    if (c == '\n') {
      Serial.print("Raw NMEA: ");
      Serial.print(nmeaSentence);

      explainNMEASentence(nmeaSentence);

      nmeaSentence = "";
    }
  }
}

// -------------------- EXPLAIN NMEA SENTENCE TYPE --------------------

void explainNMEASentence(String sentence) {
  /*
    Example sentence:
      $GNGGA,...

    G N G G A
    | | | | |
    | | GGA = fix information
    GN = mixed GNSS talker ID
  */

  if (sentence.length() < 6) {
    return;
  }

  String talker = sentence.substring(1, 3);
  String type = sentence.substring(3, 6);

  Serial.print("Talker ID: ");
  Serial.print(talker);
  Serial.print(" | Type: ");
  Serial.print(type);
  Serial.print(" | Meaning: ");

  if (type == "GGA") {
    Serial.println("Fix data: position, altitude, satellites");
  }
  else if (type == "RMC") {
    Serial.println("Recommended data: position, speed, date, time");
  }
  else if (type == "GSA") {
    Serial.println("DOP and active satellites");
  }
  else if (type == "GSV") {
    Serial.println("Satellites in view");
  }
  else if (type == "VTG") {
    Serial.println("Course and speed over ground");
  }
  else if (type == "GLL") {
    Serial.println("Latitude and longitude");
  }
  else {
    Serial.println("Other GNSS sentence");
  }

  Serial.println();
}

// -------------------- PRINT PARSED GPS DATA --------------------

void printGPSInfo() {
  Serial.println("------------- PARSED GPS DATA -------------");

  if (gps.location.isValid()) {
    Serial.print("Latitude:              ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude:             ");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("Location:              No valid fix yet");
  }

  if (gps.satellites.isValid()) {
    Serial.print("Satellites used:       ");
    Serial.println(gps.satellites.value());
  } else {
    Serial.println("Satellites used:       Waiting...");
  }

  if (gps.hdop.isValid()) {
    Serial.print("HDOP:                  ");
    Serial.println(gps.hdop.hdop());
  } else {
    Serial.println("HDOP:                  Waiting...");
  }

  if (gps.altitude.isValid()) {
    Serial.print("Altitude:              ");
    Serial.print(gps.altitude.meters());
    Serial.println(" m");
  } else {
    Serial.println("Altitude:              Waiting...");
  }

  if (gps.speed.isValid()) {
    Serial.print("Speed:                 ");
    Serial.print(gps.speed.kmph());
    Serial.println(" km/h");
  } else {
    Serial.println("Speed:                 Waiting...");
  }

  if (gps.date.isValid() && gps.time.isValid()) {
    Serial.print("Date UTC:              ");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.println(gps.date.year());

    Serial.print("Time UTC:              ");

    if (gps.time.hour() < 10) Serial.print("0");
    Serial.print(gps.time.hour());
    Serial.print(":");

    if (gps.time.minute() < 10) Serial.print("0");
    Serial.print(gps.time.minute());
    Serial.print(":");

    if (gps.time.second() < 10) Serial.print("0");
    Serial.println(gps.time.second());
  } else {
    Serial.println("Date/Time:             Waiting...");
  }

  Serial.print("Characters processed:  ");
  Serial.println(gps.charsProcessed());

  Serial.print("Sentences passed:      ");
  Serial.println(gps.passedChecksum());

  Serial.print("Sentences failed:      ");
  Serial.println(gps.failedChecksum());

  Serial.println("-------------------------------------------");
  Serial.println();
}