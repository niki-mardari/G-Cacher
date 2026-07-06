#pragma once

#include <Arduino.h>

#define TEXT_SMALL 1
#define TEXT_NORMAL 2
#define TEXT_LARGE 3

enum MenuPage
{
  MENU_HOME = 0,
  MENU_LIVE_GNSS,
  MENU_SIGNAL_QUALITY,
  MENU_GEOCACHE,
  MENU_SAVE_LOCATIONS,
  MENU_SYSTEM,
  MENU_COUNT
};

struct AppState
{
  bool bmpOK = false;
  bool qmiOK = false;
  bool sdOK = false;
  bool wifiOK = false;
  bool serverOK = false;
  bool touchOK = false;

  char sdStatusText[20] = "NOT STARTED";
  char wifiStatusText[20] = "OFFLINE";
  char serverStatusText[24] = "NOT SENT";
  char saveStatusText[48] = "No marked location saved yet";

  float bmpTempC = 0.0f;
  float bmpPressureHpa = 0.0f;
  float accX = 0.0f;
  float accY = 0.0f;
  float accZ = 0.0f;
  float accMagnitude = 0.0f;

  uint32_t gnssChars = 0;
  uint32_t lastGnssCharTime = 0;
  char nmeaLine[150] = {0};
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
  unsigned long lastWiFiRetry = 0;
  unsigned long lastNetworkAttempt = 0;
  unsigned long lastTouchAction = 0;
  unsigned long lastBootPress = 0;

  uint32_t currentWiFiRetryMs = 60000;
  uint32_t currentServerRetryMs = 30000;

  uint32_t logRows = 0;
  uint32_t uploadCount = 0;
  uint32_t uploadFailCount = 0;
  int lastHttpCode = 0;

  char serverCommand[32] = "WAITING";
  char serverColour[20] = "yellow";
  char serverMessage[150] = "Waiting for telemetry upload";
  char serverTarget[64] = "No target yet";
  float serverDistanceM = -1.0f;
  bool serverBeepLatch = false;

  int lastSavedAssetId = -1;
  bool lastSavedLocationValid = false;
  double lastSavedLat = 0.0;
  double lastSavedLon = 0.0;
  double lastSavedAlt = 0.0;
  int lastSavedSats = 0;
  float lastSavedHdop = 0.0f;
  char lastSavedName[64] = "No saved location";

  bool saveRequestPending = false;
  double pendingSaveLat = 0.0;
  double pendingSaveLon = 0.0;
  double pendingSaveAlt = 0.0;
  int pendingSaveSats = 0;
  float pendingSaveHdop = 0.0f;

  MenuPage currentMenu = MENU_HOME;
  bool forceRedraw = true;
};
