#include "UiManager.h"
#include "Config.h"
#include "GnssManager.h"
#include "Hardware.h"
#include "SensorManager.h"
#include "Utils.h"

#include <WiFi.h>
#include <string.h>

struct CachedField
{
  String text;
  uint16_t colour = TFT_WHITE;
  bool valid = false;
};

constexpr uint8_t CACHE_COUNT = 48;
CachedField cache[CACHE_COUNT];

static void resetCache()
{
  for (uint8_t i = 0; i < CACHE_COUNT; i++)
  {
    cache[i].text = "";
    cache[i].colour = TFT_WHITE;
    cache[i].valid = false;
  }
}

static void drawBackIcon(int cx, int cy, uint16_t colour)
{
  tft.fillTriangle(cx - 16, cy, cx + 5, cy - 14, cx + 5, cy + 14, colour);
  tft.fillRect(cx + 4, cy - 6, 20, 12, colour);
}

static void drawNextIcon(int cx, int cy, uint16_t colour)
{
  tft.fillTriangle(cx + 16, cy, cx - 5, cy - 14, cx - 5, cy + 14, colour);
  tft.fillRect(cx - 24, cy - 6, 20, 12, colour);
}

static void drawHomeIcon(int cx, int cy, uint16_t colour)
{
  tft.fillTriangle(cx - 18, cy - 2, cx, cy - 20, cx + 18, cy - 2, colour);
  tft.fillRect(cx - 13, cy - 2, 26, 22, colour);
  tft.fillRect(cx - 4, cy + 7, 8, 13, TFT_DARKGREY);
}

static void drawNavBar()
{
  tft.fillRect(0, NAV_Y, BIG_W, NAV_H, TFT_DARKGREY);
  tft.drawFastVLine(NAV_BACK_X1, NAV_Y, NAV_H, TFT_BLACK);
  tft.drawFastVLine(NAV_HOME_X1, NAV_Y, NAV_H, TFT_BLACK);
  drawBackIcon(53, 445, TFT_WHITE);
  drawHomeIcon(160, 445, TFT_WHITE);
  drawNextIcon(267, 445, TFT_WHITE);
}

static void drawHeader(const char *title)
{
  tft.fillRect(0, 0, BIG_W, 52, TFT_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setCursor(10, 17);
  tft.print(title);
  drawNavBar();
}

static void drawCachedText(uint8_t id, int x, int y, int w, int h, const String &text,
                           uint16_t colour = TFT_WHITE, uint8_t size = 1,
                           uint16_t background = TFT_BLACK)
{
  if (id >= CACHE_COUNT) return;

  if (cache[id].valid && cache[id].text == text && cache[id].colour == colour)
  {
    return;
  }

  tft.fillRect(x, y, w, h, background);
  tft.setTextSize(size);
  tft.setTextColor(colour, background);
  tft.setCursor(x, y);
  tft.print(text);

  cache[id].text = text;
  cache[id].colour = colour;
  cache[id].valid = true;
}

static void drawCachedBox(uint8_t id, int x, int y, int w, int h, int radius,
                          const String &text, uint16_t boxColour, uint8_t size)
{
  if (id >= CACHE_COUNT) return;

  if (cache[id].valid && cache[id].text == text && cache[id].colour == boxColour)
  {
    return;
  }

  tft.fillRoundRect(x, y, w, h, radius, boxColour);
  tft.setTextSize(size);
  tft.setTextColor(TFT_BLACK, boxColour);
  tft.setCursor(x + 18, y + ((h - (size * 8)) / 2));
  tft.print(text);

  cache[id].text = text;
  cache[id].colour = boxColour;
  cache[id].valid = true;
}

static void drawLabel(int x, int y, const char *label, uint8_t size = 2)
{
  tft.setTextSize(size);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(x, y);
  tft.print(label);
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

static String ipAddressText()
{
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return "-";
}

static String distanceText(float distanceM)
{
  if (distanceM < 0) return "--";
  if (distanceM >= 1000.0f) return String(distanceM / 1000.0f, 2) + " km";
  return String(distanceM, 1) + " m";
}

static void drawHomeMenu(AppState &app)
{
  drawHeader("GNSS SAT CACHER");

  const char *items[] = {
    "Live GNSS",
    "Signal Quality",
    "Geocache Game",
    "Save Locations",
    "System Status"
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
}

static void updateHomeValues(AppState &app)
{
  String line1 = "GNSS: ";
  line1 += qualityText(app);
  line1 += " | Sats: ";
  line1 += String(gnssSatellites());

  String line2 = "WiFi: ";
  line2 += app.wifiStatusText;
  line2 += " | Server: ";
  line2 += app.serverStatusText;

  drawCachedText(0, 18, 365, 286, 14, line1, qualityColour(app), 1);
  drawCachedText(1, 18, 383, 286, 14, line2, app.wifiOK && app.serverOK ? TFT_GREEN : TFT_YELLOW, 1);
}

static void drawLiveGnssMenu(AppState &app)
{
  (void)app;

  drawHeader("LIVE GNSS");

  constexpr int LABEL_X = 20;
  constexpr int START_Y = 64;
  constexpr int ROW_GAP = 31;

  drawLabel(LABEL_X, START_Y + ROW_GAP * 0, "Fix:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 1, "Lat:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 2, "Lon:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 3, "Alt:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 4, "Speed:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 5, "Sats:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 6, "HDOP:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 7, "Time:");
  drawLabel(LABEL_X, START_Y + ROW_GAP * 8, "Date:");

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 344);
  tft.print("Last raw NMEA:");
}

static void updateLiveGnssValues(AppState &app)
{ 
  // Spacing
  constexpr int VALUE_X = 112;
  constexpr int START_Y = 64;
  constexpr int ROW_GAP = 31;
  constexpr int VALUE_W = 190;
  constexpr int VALUE_H = 22;

  drawCachedText(2, VALUE_X, START_Y + ROW_GAP * 0, VALUE_W, VALUE_H, gnssHasFix(app) ? "YES" : "NO", gnssHasFix(app) ? TFT_GREEN : TFT_RED, 2);
  drawCachedText(3, VALUE_X, START_Y + ROW_GAP * 1, VALUE_W, VALUE_H, fixedFloat(gps.location.isValid(), gps.location.lat(), 6), TFT_WHITE, 2);
  drawCachedText(4, VALUE_X, START_Y + ROW_GAP * 2, VALUE_W, VALUE_H, fixedFloat(gps.location.isValid(), gps.location.lng(), 6), TFT_WHITE, 2);
  drawCachedText(5, VALUE_X, START_Y + ROW_GAP * 3, VALUE_W, VALUE_H, fixedFloat(gps.altitude.isValid(), gps.altitude.meters(), 1, "m"), TFT_WHITE, 2);
  drawCachedText(6, VALUE_X, START_Y + ROW_GAP * 4, VALUE_W, VALUE_H, fixedFloat(gps.speed.isValid(), gps.speed.kmph(), 1, "km/h"), TFT_WHITE, 2);
  drawCachedText(7, VALUE_X, START_Y + ROW_GAP * 5, VALUE_W, VALUE_H, String(gnssSatellites()) + " visible", TFT_WHITE, 2);
  drawCachedText(8, VALUE_X, START_Y + ROW_GAP * 6, VALUE_W, VALUE_H, fixedFloat(gps.hdop.isValid(), gnssHdop(), 2), TFT_WHITE, 2);
  drawCachedText(9, VALUE_X, START_Y + ROW_GAP * 7, VALUE_W, VALUE_H, gnssTimeText(), TFT_WHITE, 2);
  drawCachedText(10, VALUE_X, START_Y + ROW_GAP * 8, VALUE_W, VALUE_H, gnssDateText(), TFT_WHITE, 2);

  drawCachedText(11, 20, 360, 284, 48, limitText(app.lastNmeaLine, 46), TFT_YELLOW, 1);
}

static void drawSignalQualityMenu(AppState &app)
{
  drawHeader("SIGNAL QUALITY");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 68);
  tft.print("STATUS INDICATOR");

  tft.setTextSize(2);
  tft.setCursor(20, 146);
  tft.print("HDOP:");

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 190);
  tft.print("Why this status?");

  tft.setCursor(20, 250);
  tft.print("Basic FSPL estimate, theoretical only");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 276);  tft.print("Range");
  tft.setCursor(112, 276); tft.print("L1 dB");
  tft.setCursor(205, 276); tft.print("L5 dB");

  const float L1 = 1575.42f;
  const float L5 = 1176.45f;
  const float distKm[] = {0.01f, 0.1f, 1.0f};
  const char *labels[] = {"10 m", "100 m", "1 km"};

  for (int i = 0; i < 3; i++)
  {
    int y = 302 + i * 28;
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(20, y);
    tft.print(labels[i]);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(112, y);
    tft.print(calculateFSPL(distKm[i], L1), 1);
    tft.setCursor(205, y);
    tft.print(calculateFSPL(distKm[i], L5), 1);
  }
}

static void updateSignalQualityValues(AppState &app)
{
  uint16_t qColour = qualityColour(app);
  drawCachedBox(10, 18, 86, 284, 48, 8, qualityText(app), qColour, 3);

  String hdop = fixedFloat(gps.hdop.isValid(), gnssHdop(), 2);
  if (gps.hdop.isValid())
  {
    if (gnssHdop() <= 1.0f) hdop += " excellent";
    else if (gnssHdop() <= 2.0f) hdop += " good";
    else if (gnssHdop() <= 5.0f) hdop += " ok";
    else hdop += " weak";
  }

  drawCachedText(11, 100, 146, 200, 24, hdop, qColour, 2);
  drawCachedText(12, 20, 212, 282, 16, qualityReason(app), TFT_YELLOW, 1);
}

static void drawGeocacheMenu(AppState &app)
{
  drawHeader("GEOCACHE GAME");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 145);
  tft.print("Target:");
  tft.setCursor(20, 175);
  tft.print("Distance:");
  tft.setCursor(20, 215);
  tft.print("Message:");
  tft.setCursor(20, 330);
  tft.print("Use Save Locations menu to mark");
  tft.setCursor(20, 348);
  tft.print("your current GNSS position.");
}

static void updateGeocacheValues(AppState &app)
{
  drawCachedBox(13, 18, 70, 284, 54, 8, app.serverCommand, colourFromServer(app.serverColour), 2);
  drawCachedText(14, 80, 145, 220, 16, limitText(app.serverTarget, 32), TFT_YELLOW, 1);
  drawCachedText(15, 90, 175, 210, 16, distanceText(app.serverDistanceM), TFT_WHITE, 1);
  drawCachedText(16, 20, 238, 282, 60, limitText(app.serverMessage, 115), TFT_CYAN, 1);
}

static void drawSaveLocationsMenu(AppState &app)
{
  drawHeader("SAVE LOCATIONS");

  tft.drawRoundRect(SAVE_BTN_X, SAVE_BTN_Y, SAVE_BTN_W, SAVE_BTN_H, 8, TFT_CYAN);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(42, SAVE_BTN_Y + 14);
  tft.print("Save current GNSS");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 140);
  tft.print("Most recent saved location:");
}

static void updateSaveLocationsValues(AppState &app)
{
  if (!app.lastSavedLocationValid)
  {
    drawCachedText(17, 20, 170, 280, 16, "No marked location saved yet.", TFT_YELLOW, 1);
    drawCachedText(18, 20, 195, 280, 16, "Wait for GNSS fix, then press", TFT_YELLOW, 1);
    drawCachedText(19, 20, 213, 280, 16, "Save current GNSS.", TFT_YELLOW, 1);
    drawCachedText(20, 20, 240, 280, 110, "", TFT_WHITE, 1);
  }
  else
  {
    drawCachedText(17, 20, 168, 286, 16, String("Name: ") + limitText(app.lastSavedName, 34), TFT_YELLOW, 1);
    drawCachedText(18, 20, 192, 286, 16, String("Asset ID: ") + String(app.lastSavedAssetId), TFT_WHITE, 1);
    drawCachedText(19, 20, 216, 286, 16, String("Lat: ") + String(app.lastSavedLat, 6), TFT_WHITE, 1);
    drawCachedText(20, 20, 240, 286, 16, String("Lon: ") + String(app.lastSavedLon, 6), TFT_WHITE, 1);
    drawCachedText(21, 20, 264, 286, 16, String("Alt: ") + String(app.lastSavedAlt, 1) + " m", TFT_WHITE, 1);
    drawCachedText(22, 20, 288, 286, 16, String("Sats: ") + String(app.lastSavedSats), TFT_WHITE, 1);
    drawCachedText(23, 20, 312, 286, 16, String("HDOP: ") + String(app.lastSavedHdop, 2), TFT_WHITE, 1);
  }

  drawCachedText(24, 20, 360, 286, 16, app.saveStatusText, TFT_CYAN, 1);
}

static void drawSystemMenu(AppState &app)
{
  drawHeader("SYSTEM STATUS");
}

static void updateSystemValues(AppState &app)
{
  int y = 68;
  drawCachedText(25, 18, y, 286, 14, String("BMP280: ") + (app.bmpOK ? "OK" : "NO"), app.bmpOK ? TFT_GREEN : TFT_RED, 1);
  y += 20;
  drawCachedText(26, 18, y, 286, 14, String("QMI8658C: ") + (app.qmiOK ? "OK" : "NO"), app.qmiOK ? TFT_GREEN : TFT_RED, 1);
  y += 20;
  drawCachedText(27, 18, y, 286, 14, String("SD Card: ") + app.sdStatusText, app.sdOK ? TFT_GREEN : TFT_RED, 1);
  y += 20;
  drawCachedText(28, 18, y, 286, 14, String("Touch: ") + (app.touchOK ? "OK" : "WAIT"), app.touchOK ? TFT_GREEN : TFT_YELLOW, 1);
  y += 20;
  drawCachedText(29, 18, y, 286, 14, String("GNSS UART: ") + (gnssReceiving(app) ? "OK" : "NO DATA"), gnssReceiving(app) ? TFT_GREEN : TFT_RED, 1);

  y += 32;
  drawCachedText(30, 18, y, 286, 14, String("WiFi: ") + app.wifiStatusText, app.wifiOK ? TFT_GREEN : TFT_RED, 1);
  y += 20;
  drawCachedText(31, 18, y, 286, 14, String("ESP32 IP: ") + ipAddressText(), TFT_WHITE, 1);
  y += 20;
  drawCachedText(32, 18, y, 286, 14, String("Server IP: ") + (strlen(SERVER_HOST) ? SERVER_HOST : "not set"), TFT_WHITE, 1);
  y += 20;
  drawCachedText(33, 18, y, 286, 14, String("Server: ") + app.serverStatusText, app.serverOK ? TFT_GREEN : TFT_RED, 1);
  y += 20;
  drawCachedText(34, 18, y, 286, 14, String("HTTP: ") + String(app.lastHttpCode) + "  Uploads: " + String(app.uploadCount), TFT_WHITE, 1);

  y += 32;
  drawCachedText(35, 18, y, 286, 14, String("Temp: ") + String(app.bmpTempC, 1) + " C " + thermalStatus(app), thermalColour(app), 1);
  y += 20;
  drawCachedText(36, 18, y, 286, 14, String("Pressure: ") + String(app.bmpPressureHpa, 1) + " hPa", TFT_WHITE, 1);
  y += 20;
  drawCachedText(37, 18, y, 286, 14, String("AX:") + String(app.accX, 2) + " AY:" + String(app.accY, 2) + " AZ:" + String(app.accZ, 2), TFT_WHITE, 1);
  y += 20;
  drawCachedText(38, 18, y, 286, 14, String("Logs: ") + String(app.logRows) + "  File: " + LOG_FILE, TFT_WHITE, 1);
  y += 20;
  drawCachedText(39, 18, y, 286, 14, String("Touch raw: ") + String(app.rawTouchX) + "," + String(app.rawTouchY), TFT_WHITE, 1);
}

static void updateScreenValues(AppState &app)
{
  switch (app.currentMenu)
  {
    case MENU_HOME: updateHomeValues(app); break;
    case MENU_LIVE_GNSS: updateLiveGnssValues(app); break;
    case MENU_SIGNAL_QUALITY: updateSignalQualityValues(app); break;
    case MENU_GEOCACHE: updateGeocacheValues(app); break;
    case MENU_SAVE_LOCATIONS: updateSaveLocationsValues(app); break;
    case MENU_SYSTEM: updateSystemValues(app); break;
    default: break;
  }
}

void drawCurrentScreen(AppState &app)
{
  resetCache();
  tft.fillScreen(TFT_BLACK);

  switch (app.currentMenu)
  {
    case MENU_HOME: drawHomeMenu(app); break;
    case MENU_LIVE_GNSS: drawLiveGnssMenu(app); break;
    case MENU_SIGNAL_QUALITY: drawSignalQualityMenu(app); break;
    case MENU_GEOCACHE: drawGeocacheMenu(app); break;
    case MENU_SAVE_LOCATIONS: drawSaveLocationsMenu(app); break;
    case MENU_SYSTEM: drawSystemMenu(app); break;
    default: drawHomeMenu(app); break;
  }

  updateScreenValues(app);
}

void updateUi(AppState &app)
{
  if (app.forceRedraw)
  {
    drawCurrentScreen(app);
    app.forceRedraw = false;
    app.lastDisplayUpdate = millis();
    return;
  }

  if (millis() - app.lastDisplayUpdate < DISPLAY_UPDATE_MS) return;
  app.lastDisplayUpdate = millis();
  updateScreenValues(app);
}
