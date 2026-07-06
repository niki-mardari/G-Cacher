/********************************************************
  ESP32-S3 + LC29H GNSS + FastAPI Telemetry Upload

  Purpose:
  - Minimal example for SatCacher backend API upload
  - No display, no touch, no SD card, no sensors, no buzzer
  - Reads GNSS using TinyGPSPlus
  - Connects to Wi-Fi
  - POSTs telemetry JSON to FastAPI /api/telemetry

  Wiring used in your project:
  - LC29H TX  -> ESP32-S3 GPIO17  // ESP32 receives GNSS data
  - LC29H RX  -> ESP32-S3 GPIO18  // ESP32 can transmit to GNSS, optional here
  - LC29H GND -> ESP32-S3 GND
  - LC29H powered by suitable 5 V / USB supply

  Serial Monitor:
  - 115200 baud
********************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>

// ------------------------------------------------------------
// User settings
// ------------------------------------------------------------

const char *WIFI_SSID = ""; // Put your Wi-Fi SSID here
const char *WIFI_PASSWORD = ""; // Put your Wi-Fi password here

// Change this if your laptop/server IP changes.
// FastAPI endpoint used by the ESP32:
//   POST http://<server-ip>:8000/api/telemetry
const char *SERVER_HOST = ""; // Replace with your FastAPI server IP
// Replace with "http://<server_ip>:8000/api/telemetry"

// Set this to true when testing indoors without a GNSS fix.
// It uploads a demo point near TU Dublin Blanchardstown-style test area.
// Set to false for real outdoor GNSS readings.
const bool USE_DEMO_LOCATION_WHEN_NO_FIX = true;

// Example/demo coordinates. Replace with your exact test point if needed.
const double DEMO_LAT = 53.4050;
const double DEMO_LON = -6.3780;
const double DEMO_ALT_M = 80.0;

// Set true to print every raw NMEA sentence from the LC29H.
const bool PRINT_RAW_NMEA = true;

// ------------------------------------------------------------
// GNSS UART settings
// ------------------------------------------------------------

#define GNSS_RX_PIN 17
#define GNSS_TX_PIN 18
#define GNSS_BAUD   115200

HardwareSerial GNSS(1);
TinyGPSPlus gps;

// ------------------------------------------------------------
// Timing
// ------------------------------------------------------------

const unsigned long WIFI_RETRY_MS     = 10000;
const unsigned long STATUS_PRINT_MS   = 2000;
const unsigned long UPLOAD_INTERVAL_MS = 5000;
const unsigned long HTTP_TIMEOUT_MS   = 5000;

unsigned long lastWiFiAttempt = 0;
unsigned long lastStatusPrint = 0;
unsigned long lastUpload = 0;

// ------------------------------------------------------------
// State/debug counters
// ------------------------------------------------------------

String currentNmeaLine = "";
String lastNmeaLine = "";
uint32_t nmeaLineCount = 0;
uint32_t uploadCount = 0;
uint32_t uploadFailCount = 0;
int lastHttpCode = 0;

// ------------------------------------------------------------
// Wi-Fi
// ------------------------------------------------------------

void connectWiFiIfNeeded()
{
  if (WiFi.status() == WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastWiFiAttempt < WIFI_RETRY_MS) return;
  lastWiFiAttempt = now;

  Serial.println();
  Serial.println("[WiFi] Connecting...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// ------------------------------------------------------------
// GNSS reading
// ------------------------------------------------------------

void readGNSS()
{
  while (GNSS.available())
  {
    char c = (char)GNSS.read();

    // TinyGPSPlus parses the NMEA stream character by character.
    gps.encode(c);

    // Keep complete raw NMEA strings for Serial Monitor learning/debugging.
    if (c == '\n')
    {
      currentNmeaLine.trim();

      if (currentNmeaLine.length() > 0)
      {
        lastNmeaLine = currentNmeaLine;
        nmeaLineCount++;

        if (PRINT_RAW_NMEA)
        {
          Serial.print("[RAW NMEA #");
          Serial.print(nmeaLineCount);
          Serial.print("] ");
          Serial.println(lastNmeaLine);
        }
      }

      currentNmeaLine = "";
    }
    else if (c != '\r')
    {
      if (currentNmeaLine.length() < 140)
      {
        currentNmeaLine += c;
      }
    }
  }
}

bool hasFreshFix()
{
  return gps.location.isValid() && gps.location.age() < 5000;
}

int satelliteCount()
{
  return gps.satellites.isValid() ? gps.satellites.value() : 0;
}

double hdopValue()
{
  return gps.hdop.isValid() ? gps.hdop.hdop() : 99.99;
}

// ------------------------------------------------------------
// Serial status
// ------------------------------------------------------------

void printStatus()
{
  unsigned long now = millis();
  if (now - lastStatusPrint < STATUS_PRINT_MS) return;
  lastStatusPrint = now;

  bool fix = hasFreshFix();

  Serial.println();
  Serial.println("================ ESP32-S3 GNSS Telemetry Status ================");
  Serial.print("WiFi: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "NOT CONNECTED");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  }

  Serial.print("Server Host: ");
  Serial.println(SERVER_HOST);
  Serial.print("GNSS fix: ");
  Serial.println(fix ? "YES" : "NO");
  Serial.print("Satellites visible: ");
  Serial.println(satelliteCount());
  Serial.print("HDOP: ");
  Serial.println(hdopValue(), 2);

  if (fix)
  {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude m: ");
    Serial.println(gps.altitude.isValid() ? gps.altitude.meters() : 0.0, 2);
    Serial.print("Speed km/h: ");
    Serial.println(gps.speed.isValid() ? gps.speed.kmph() : 0.0, 2);
  }
  else if (USE_DEMO_LOCATION_WHEN_NO_FIX)
  {
    Serial.println("No GNSS fix yet, so demo location will be uploaded for API testing.");
    Serial.print("Demo lat/lon: ");
    Serial.print(DEMO_LAT, 6);
    Serial.print(", ");
    Serial.println(DEMO_LON, 6);
  }

  Serial.print("Last HTTP code: ");
  Serial.println(lastHttpCode);
  Serial.print("Uploads OK: ");
  Serial.println(uploadCount);
  Serial.print("Uploads failed: ");
  Serial.println(uploadFailCount);
  Serial.print("Last NMEA: ");
  Serial.println(lastNmeaLine.length() ? lastNmeaLine : "No NMEA yet");
  Serial.println("================================================================");
}

// ------------------------------------------------------------
// FastAPI telemetry upload
// ------------------------------------------------------------

bool buildTelemetryJson(String &jsonOut)
{
  bool realFix = hasFreshFix();
  bool useDemo = (!realFix && USE_DEMO_LOCATION_WHEN_NO_FIX);

  if (!realFix && !useDemo)
  {
    Serial.println("[Upload] Skipped: no GNSS fix yet.");
    return false;
  }

  double lat = realFix ? gps.location.lat() : DEMO_LAT;
  double lon = realFix ? gps.location.lng() : DEMO_LON;
  double alt = realFix && gps.altitude.isValid() ? gps.altitude.meters() : DEMO_ALT_M;
  double speed = realFix && gps.speed.isValid() ? gps.speed.kmph() : 0.0;

  StaticJsonDocument<512> doc;

  // These names match the FastAPI backend telemetry fields.
  doc["device_id"] = "tenstar_001";
  doc["mode"] = useDemo ? "api_demo_tud_blanch" : "geocache";
  doc["lat"] = lat;
  doc["lon"] = lon;
  doc["alt_m"] = alt;
  doc["speed_kmph"] = speed;
  doc["satellites"] = satelliteCount();
  doc["hdop"] = hdopValue();
  doc["fix"] = realFix || useDemo;

  // No extra peripherals in this simple sketch.
  // These are included because the backend currently accepts them.
  doc["temperature_c"] = 0.0;
  doc["pressure_hpa"] = 0.0;
  doc["acc_x"] = 0.0;
  doc["acc_y"] = 0.0;
  doc["acc_z"] = 0.0;

  serializeJson(doc, jsonOut);
  return true;
}

void uploadTelemetry()
{
  unsigned long now = millis();
  if (now - lastUpload < UPLOAD_INTERVAL_MS) return;
  lastUpload = now;

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[Upload] Skipped: WiFi not connected.");
    uploadFailCount++;
    return;
  }

  String jsonBody;
  if (!buildTelemetryJson(jsonBody)) return;

  Serial.println();
  Serial.println("[HTTP POST] Uploading telemetry to FastAPI...");
  Serial.print("Host: ");
  Serial.println(SERVER_HOST);
  Serial.print("JSON: ");
  Serial.println(jsonBody);

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.begin(SERVER_HOST);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(jsonBody);
  lastHttpCode = httpCode;

  String response = http.getString();
  http.end();

  Serial.print("HTTP code: ");
  Serial.println(httpCode);
  Serial.print("Response: ");
  Serial.println(response);

  if (httpCode >= 200 && httpCode < 300)
  {
    uploadCount++;

    // Optional: parse the command returned by the SatCacher backend.
    StaticJsonDocument<512> reply;
    DeserializationError err = deserializeJson(reply, response);

    if (!err)
    {
      const char *command = reply["command"] | "";
      const char *colour = reply["screen_color"] | "";
      const char *target = reply["target_name"] | "";
      const char *message = reply["message"] | "";
      double distance = reply["distance_m"] | -1.0;

      Serial.println("[Backend command parsed]");
      Serial.print("Command: ");
      Serial.println(command);
      Serial.print("Colour: ");
      Serial.println(colour);
      Serial.print("Target: ");
      Serial.println(target);
      Serial.print("Distance m: ");
      Serial.println(distance, 1);
      Serial.print("Message: ");
      Serial.println(message);
    }
  }
  else
  {
    uploadFailCount++;
  }
}

// ------------------------------------------------------------
// Arduino setup/loop
// ------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println("ESP32-S3 + LC29H Simple FastAPI Telemetry Upload");
  Serial.println("No display, no touch, no SD, no sensors");
  Serial.println("============================================================");

  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);

  Serial.println("GNSS UART started");
  Serial.print("LC29H TX -> ESP32 GPIO");
  Serial.print(GNSS_RX_PIN);
  Serial.println(" RX");
  Serial.print("LC29H RX -> ESP32 GPIO");
  Serial.print(GNSS_TX_PIN);
  Serial.println(" TX");
  Serial.print("GNSS baud: ");
  Serial.println(GNSS_BAUD);

  // Start first Wi-Fi attempt immediately.
  lastWiFiAttempt = millis() - WIFI_RETRY_MS;
  connectWiFiIfNeeded();
}

void loop()
{
  readGNSS();
  connectWiFiIfNeeded();
  printStatus();
  uploadTelemetry();
}
