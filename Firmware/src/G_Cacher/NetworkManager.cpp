#include "NetworkManager.h"
#include "Buzzer.h"
#include "Config.h"
#include "GnssManager.h"
#include "Hardware.h"
#include "Utils.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <string.h>

static bool wifiConfigured()
{
  return strlen(WIFI_SSID) > 0;
}

static bool telemetryConfigured()
{
  return strlen(TELEMETRY_URL) > 0;
}

static bool assetsConfigured()
{
  return strlen(ASSETS_URL) > 0;
}

static void increaseWiFiBackoff(AppState &app)
{
  app.currentWiFiRetryMs *= 2;
  if (app.currentWiFiRetryMs > WIFI_RETRY_MAX_MS) app.currentWiFiRetryMs = WIFI_RETRY_MAX_MS;
}

static void resetWiFiBackoff(AppState &app)
{
  app.currentWiFiRetryMs = WIFI_RETRY_START_MS;
}

static void increaseServerBackoff(AppState &app)
{
  app.currentServerRetryMs *= 2;
  if (app.currentServerRetryMs > SERVER_RETRY_MAX_MS) app.currentServerRetryMs = SERVER_RETRY_MAX_MS;
}

static void resetServerBackoff(AppState &app)
{
  app.currentServerRetryMs = SERVER_RETRY_START_MS;
}

static bool startHttpPost(const char *url, const String &body, String *response, int *httpCode)
{
  if (url == nullptr || strlen(url) == 0)
  {
    if (httpCode != nullptr) *httpCode = 0;
    return false;
  }

  WiFiClient client;
  client.setTimeout(HTTP_READ_TIMEOUT_MS);

  HTTPClient http;
  http.setTimeout(HTTP_READ_TIMEOUT_MS);
  http.setConnectTimeout(HTTP_CONNECT_TIMEOUT_MS);

  if (!http.begin(client, url))
  {
    if (httpCode != nullptr) *httpCode = 0;
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);

  if (httpCode != nullptr) *httpCode = code;

  bool ok = code >= 200 && code < 300;
  if (ok && response != nullptr)
  {
    *response = http.getString();
  }

  http.end();
  return ok;
}

static void parseServerReply(AppState &app, const String &response)
{
  StaticJsonDocument<1024> reply;
  DeserializationError err = deserializeJson(reply, response);
  if (err)
  {
    safeCopy(app.serverStatusText, sizeof(app.serverStatusText), "JSON ERR");
    app.serverOK = false;
    return;
  }

  char previousTarget[sizeof(app.serverTarget)];
  safeCopy(previousTarget, sizeof(previousTarget), app.serverTarget);

  safeCopy(app.serverCommand, sizeof(app.serverCommand), reply["command"] | "WAITING");
  safeCopy(app.serverColour, sizeof(app.serverColour), reply["screen_color"] | "yellow");
  safeCopy(app.serverMessage, sizeof(app.serverMessage), reply["message"] | "No message");

  const char *targetName = reply["target_name"] | nullptr;
  if (targetName == nullptr) targetName = reply["title"] | "No target";
  safeCopy(app.serverTarget, sizeof(app.serverTarget), targetName);

  if (strcmp(previousTarget, app.serverTarget) != 0)
  {
    app.serverBeepLatch = false;
  }

  app.serverDistanceM = reply["distance_m"] | -1.0f;

  bool doBeep = reply["beep"] | false;
  if (doBeep && !app.serverBeepLatch)
  {
    playTargetReachedMelody();
    app.serverBeepLatch = true;
  }
  else if (!doBeep)
  {
    app.serverBeepLatch = false;
  }
}

static bool uploadTelemetry(AppState &app)
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
  doc["fix"] = gnssHasFix(app);

  if (app.bmpOK)
  {
    doc["temperature_c"] = app.bmpTempC;
    doc["pressure_hpa"] = app.bmpPressureHpa;
  }

  if (app.qmiOK)
  {
    doc["acc_x"] = app.accX;
    doc["acc_y"] = app.accY;
    doc["acc_z"] = app.accZ;
  }

  String body;
  serializeJson(doc, body);

  String response;
  int code = 0;
  bool ok = startHttpPost(TELEMETRY_URL, body, &response, &code);
  app.lastHttpCode = code;

  if (ok)
  {
    app.serverOK = true;
    app.uploadCount++;
    safeCopy(app.serverStatusText, sizeof(app.serverStatusText), "OK");
    resetServerBackoff(app);
    parseServerReply(app, response);
  }
  else
  {
    app.serverOK = false;
    app.uploadFailCount++;
    safeCopy(app.serverStatusText, sizeof(app.serverStatusText), "ERROR");
    increaseServerBackoff(app);
  }

  return ok;
}

static bool sendPendingSave(AppState &app)
{
  if (!app.saveRequestPending) return true;
  if (!assetsConfigured())
  {
    safeCopy(app.saveStatusText, sizeof(app.saveStatusText), "Assets URL not configured.");
    app.saveRequestPending = false;
    return false;
  }

  StaticJsonDocument<512> doc;
  doc["device_id"] = DEVICE_ID;
  doc["asset_type"] = "marker";
  doc["name"] = "Marked spot from GNSS Sat Cacher";
  doc["lat"] = app.pendingSaveLat;
  doc["lon"] = app.pendingSaveLon;
  doc["alt_m"] = app.pendingSaveAlt;
  doc["description"] = "Saved from the GNSS Sat Cacher device. Rename or tag this on the web dashboard.";
  doc["satellites"] = app.pendingSaveSats;
  doc["hdop"] = app.pendingSaveHdop;

  String body;
  serializeJson(doc, body);

  String response;
  int code = 0;
  bool ok = startHttpPost(ASSETS_URL, body, &response, &code);
  app.lastHttpCode = code;

  if (ok)
  {
    StaticJsonDocument<768> reply;
    DeserializationError saveErr = deserializeJson(reply, response);
    if (!saveErr)
    {
      app.lastSavedAssetId = reply["asset"]["id"] | reply["id"] | -1;
      const char *savedName = reply["asset"]["name"] | reply["name"] | "Marked spot from GNSS Sat Cacher";
      safeCopy(app.lastSavedName, sizeof(app.lastSavedName), savedName);
    }
    else
    {
      app.lastSavedAssetId = -1;
      safeCopy(app.lastSavedName, sizeof(app.lastSavedName), "Marked spot from GNSS Sat Cacher");
    }

    app.lastSavedLocationValid = true;
    app.lastSavedLat = app.pendingSaveLat;
    app.lastSavedLon = app.pendingSaveLon;
    app.lastSavedAlt = app.pendingSaveAlt;
    app.lastSavedSats = app.pendingSaveSats;
    app.lastSavedHdop = app.pendingSaveHdop;

    safeCopy(app.saveStatusText, sizeof(app.saveStatusText), "Saved to backend. Rename on site.");
    clickBeep();
  }
  else
  {
    safeCopy(app.saveStatusText, sizeof(app.saveStatusText), "Save failed. Check backend.");
    errorBeep();
  }

  app.saveRequestPending = false;
  return ok;
}

void initNetwork(AppState &app)
{
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);

  app.currentWiFiRetryMs = WIFI_RETRY_START_MS;
  app.currentServerRetryMs = SERVER_RETRY_START_MS;

  if (!wifiConfigured())
  {
    app.wifiOK = false;
    app.serverOK = false;
    safeCopy(app.wifiStatusText, sizeof(app.wifiStatusText), "NOT SET");
    safeCopy(app.serverStatusText, sizeof(app.serverStatusText), "DISABLED");
    return;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  safeCopy(app.wifiStatusText, sizeof(app.wifiStatusText), "CONNECTING");
  app.lastWiFiRetry = millis();
}

void updateNetwork(AppState &app)
{
  if (!wifiConfigured())
  {
    app.wifiOK = false;
    app.serverOK = false;
    safeCopy(app.wifiStatusText, sizeof(app.wifiStatusText), "NOT SET");
    safeCopy(app.serverStatusText, sizeof(app.serverStatusText), "DISABLED");
    return;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    if (!app.wifiOK) resetWiFiBackoff(app);
    app.wifiOK = true;
    safeCopy(app.wifiStatusText, sizeof(app.wifiStatusText), "OK");
  }
  else
  {
    app.wifiOK = false;
    app.serverOK = false;
    safeCopy(app.wifiStatusText, sizeof(app.wifiStatusText), "OFFLINE");
    safeCopy(app.serverStatusText, sizeof(app.serverStatusText), "NO WIFI");

    if (millis() - app.lastWiFiRetry >= app.currentWiFiRetryMs)
    {
      app.lastWiFiRetry = millis();
      safeCopy(app.wifiStatusText, sizeof(app.wifiStatusText), "CONNECTING");
      WiFi.disconnect(false);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      increaseWiFiBackoff(app);
    }
    return;
  }

  if (!telemetryConfigured())
  {
    app.serverOK = false;
    safeCopy(app.serverStatusText, sizeof(app.serverStatusText), "NOT SET");
    return;
  }

  uint32_t waitMs = app.serverOK ? UPLOAD_INTERVAL_MS : app.currentServerRetryMs;
  if (millis() - app.lastNetworkAttempt < waitMs) return;
  app.lastNetworkAttempt = millis();

  if (app.saveRequestPending)
  {
    sendPendingSave(app);
  }
  else
  {
    uploadTelemetry(app);
  }
}

void requestSaveLocation(AppState &app)
{
  if (!gnssHasFix(app))
  {
    safeCopy(app.saveStatusText, sizeof(app.saveStatusText), "No GNSS fix. Cannot save.");
    errorBeep();
    return;
  }

  if (!app.wifiOK)
  {
    safeCopy(app.saveStatusText, sizeof(app.saveStatusText), "No WiFi. Cannot save.");
    errorBeep();
    return;
  }

  if (!assetsConfigured())
  {
    safeCopy(app.saveStatusText, sizeof(app.saveStatusText), "Assets URL not configured.");
    errorBeep();
    return;
  }

  app.pendingSaveLat = gps.location.lat();
  app.pendingSaveLon = gps.location.lng();
  app.pendingSaveAlt = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
  app.pendingSaveSats = gnssSatellites();
  app.pendingSaveHdop = gps.hdop.isValid() ? gnssHdop() : 99.99f;
  app.saveRequestPending = true;

  safeCopy(app.saveStatusText, sizeof(app.saveStatusText), "Save queued for backend.");
  clickBeep();
}
