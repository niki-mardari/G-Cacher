#include "StorageManager.h"
#include "Config.h"
#include "GnssManager.h"
#include "Hardware.h"
#include "Utils.h"
#include <SD.h>

static bool tryStartSD(uint32_t frequency)
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

static void createLogHeaderIfNeeded(AppState &app)
{
  if (!app.sdOK) return;

  bool exists = SD.exists(LOG_FILE);
  File file = SD.open(LOG_FILE, FILE_APPEND);
  if (!file)
  {
    app.sdOK = false;
    safeCopy(app.sdStatusText, sizeof(app.sdStatusText), "FILE FAIL");
    return;
  }

  if (!exists || file.size() == 0)
  {
    file.println("millis,fix,lat,lon,alt_m,speed_kmph,satellites,hdop,quality,temp_c,pressure_hpa,acc_x,acc_y,acc_z,wifi,server,http_code,command,target,distance_m");
  }
  file.close();
}

static void printFloatOrBlank(File &file, bool valid, double value, int decimals)
{
  if (valid) file.print(value, decimals);
}

static void appendLogRow(AppState &app)
{
  File file = SD.open(LOG_FILE, FILE_APPEND);
  if (!file)
  {
    app.sdOK = false;
    safeCopy(app.sdStatusText, sizeof(app.sdStatusText), "LOG FAIL");
    return;
  }

  file.print(millis()); file.print(',');
  file.print(gnssHasFix(app) ? "YES" : "NO"); file.print(',');
  printFloatOrBlank(file, gps.location.isValid(), gps.location.lat(), 6); file.print(',');
  printFloatOrBlank(file, gps.location.isValid(), gps.location.lng(), 6); file.print(',');
  printFloatOrBlank(file, gps.altitude.isValid(), gps.altitude.meters(), 1); file.print(',');
  printFloatOrBlank(file, gps.speed.isValid(), gps.speed.kmph(), 1); file.print(',');
  file.print(gnssSatellites()); file.print(',');
  printFloatOrBlank(file, gps.hdop.isValid(), gnssHdop(), 2); file.print(',');
  file.print(qualityText(app)); file.print(',');
  printFloatOrBlank(file, app.bmpOK, app.bmpTempC, 1); file.print(',');
  printFloatOrBlank(file, app.bmpOK, app.bmpPressureHpa, 1); file.print(',');
  printFloatOrBlank(file, app.qmiOK, app.accX, 3); file.print(',');
  printFloatOrBlank(file, app.qmiOK, app.accY, 3); file.print(',');
  printFloatOrBlank(file, app.qmiOK, app.accZ, 3); file.print(',');
  file.print(app.wifiStatusText); file.print(',');
  file.print(app.serverStatusText); file.print(',');
  file.print(app.lastHttpCode); file.print(',');
  file.print(app.serverCommand); file.print(',');
  file.print(app.serverTarget); file.print(',');
  if (app.serverDistanceM >= 0) file.print(app.serverDistanceM, 1);
  file.println();
  file.close();

  app.logRows++;
}

void initStorage(AppState &app)
{
  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  app.sdOK = tryStartSD(SD_FREQ_SLOW);
  if (!app.sdOK) app.sdOK = tryStartSD(SD_FREQ_MED);
  if (!app.sdOK) app.sdOK = tryStartSD(SD_FREQ_FAST);

  if (app.sdOK)
  {
    safeCopy(app.sdStatusText, sizeof(app.sdStatusText), "OK");
    createLogHeaderIfNeeded(app);
    Serial.println("SD card OK");
  }
  else
  {
    safeCopy(app.sdStatusText, sizeof(app.sdStatusText), "FAILED");
    Serial.println("SD card failed");
  }
}

void updateStorage(AppState &app)
{
  if (!app.sdOK) return;
  if (millis() - app.lastLogTime < LOG_INTERVAL_MS) return;

  app.lastLogTime = millis();
  appendLogRow(app);
}
