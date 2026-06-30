#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Change these
const char *ssid = "yep";
const char *password = "12345678";

// IMPORTANT:
// Do not use 127.0.0.1 here.
// Both Esp32S3 and Laptop have to be connected to the same Network
// For Esp32S3 the network has to be configured for 2.4GHz
// Use your laptop's Wi-Fi/hotspot IP address.
const char *serverUrl = "http://10.107.7.42:8000/api/telemetry";

unsigned long lastUploadTime = 0;
const unsigned long UPLOAD_INTERVAL_MS = 5000;

void connectWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return;
  }

  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi connection failed");
  }
}

void uploadTelemetry()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();

    if (WiFi.status() != WL_CONNECTED)
    {
      return;
    }
  }

  StaticJsonDocument<512> doc;

  // Dummy test data
  doc["device_id"] = "tenstar_001";
  doc["mode"] = "geocache";

  doc["lat"] = 53.349805;
  doc["lon"] = -6.260310;
  doc["alt_m"] = 40.5;
  doc["speed_kmph"] = 2.1;

  doc["satellites"] = 35;
  doc["hdop"] = 0.8;
  doc["fix"] = true;

  doc["temperature_c"] = 39.8;
  doc["pressure_hpa"] = 1012.4;

  doc["acc_x"] = 0.1;
  doc["acc_y"] = 0.0;
  doc["acc_z"] = 9.8;

  String body;
  serializeJson(doc, body);

  Serial.println();
  Serial.println("Uploading telemetry:");
  Serial.println(body);

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(body);

  Serial.print("HTTP code: ");
  Serial.println(httpCode);

  if (httpCode > 0)
  {
    String response = http.getString();

    Serial.println("Server response:");
    Serial.println(response);

    StaticJsonDocument<512> reply;
    DeserializationError err = deserializeJson(reply, response);

    if (!err)
    {
      const char *command = reply["command"] | "";
      const char *screenColor = reply["screen_color"] | "";
      const char *title = reply["title"] | "";
      const char *message = reply["message"] | "";
      float distanceM = reply["distance_m"] | -1.0;

      Serial.println("Parsed command:");
      Serial.print("Command: ");
      Serial.println(command);

      Serial.print("Screen colour: ");
      Serial.println(screenColor);

      Serial.print("Title: ");
      Serial.println(title);

      Serial.print("Message: ");
      Serial.println(message);

      Serial.print("Distance: ");
      Serial.println(distanceM);
    }
    else
    {
      Serial.print("JSON parse failed: ");
      Serial.println(err.c_str());
    }
  }
  else
  {
    Serial.print("HTTP POST failed: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

void setup()
{
  Serial.begin(115200);
  delay(1500);

  Serial.println("ESP32-S3 FastAPI upload test");

  connectWiFi();
  uploadTelemetry();
}

void loop()
{
  if (millis() - lastUploadTime >= UPLOAD_INTERVAL_MS)
  {
    lastUploadTime = millis();
    uploadTelemetry();
  }
}