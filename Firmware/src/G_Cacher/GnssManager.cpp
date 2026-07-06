#include "GnssManager.h"
#include "Config.h"
#include "Hardware.h"
#include <string.h>

void initGnss(AppState &app)
{
  GNSS.setRxBufferSize(2048);
  GNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  app.lastGnssCharTime = 0;
  Serial.println("LC29H UART started");
}

void updateGnss(AppState &app)
{
  while (GNSS.available())
  {
    char c = GNSS.read();
    gps.encode(c);
    app.gnssChars++;
    app.lastGnssCharTime = millis();

    if (c == '\n')
    {
      app.nmeaLine[app.nmeaIndex] = '\0';
      if (app.nmeaIndex > 5)
      {
        strncpy(app.lastNmeaLine, app.nmeaLine, sizeof(app.lastNmeaLine) - 1);
        app.lastNmeaLine[sizeof(app.lastNmeaLine) - 1] = '\0';
      }
      app.nmeaIndex = 0;
    }
    else if (c != '\r')
    {
      if (app.nmeaIndex < sizeof(app.nmeaLine) - 1)
      {
        app.nmeaLine[app.nmeaIndex++] = c;
      }
      else
      {
        app.nmeaIndex = 0;
      }
    }
  }
}

bool gnssReceiving(const AppState &app)
{
  return millis() - app.lastGnssCharTime < 3000;
}

bool gnssHasFix(const AppState &app)
{
  (void)app;
  return gps.location.isValid() && gps.location.age() < 5000;
}

int gnssSatellites()
{
  if (gps.satellites.isValid()) return gps.satellites.value();
  return 0;
}

float gnssHdop()
{
  if (gps.hdop.isValid()) return gps.hdop.hdop();
  return 99.99f;
}

const char *qualityText(const AppState &app)
{
  if (!gnssReceiving(app)) return "NO DATA";
  if (!gnssHasFix(app)) return "POOR";
  if (gnssSatellites() >= 8 && gnssHdop() <= 2.0f) return "GOOD";
  if (gnssSatellites() >= 4 && gnssHdop() <= 5.0f) return "OK";
  return "POOR";
}

const char *qualityReason(const AppState &app)
{
  if (!gnssReceiving(app)) return "No NMEA data from LC29H";
  if (!gnssHasFix(app)) return "Waiting for a valid GNSS fix";
  if (gnssSatellites() < 4) return "Too few satellites";
  if (gnssHdop() > 5.0f) return "HDOP is high";
  if (gnssSatellites() >= 8 && gnssHdop() <= 2.0f) return "Strong fix and low HDOP";
  return "Usable fix, but not excellent";
}

uint16_t qualityColour(const AppState &app)
{
  const char *q = qualityText(app);
  if (strcmp(q, "GOOD") == 0) return TFT_GREEN;
  if (strcmp(q, "OK") == 0) return TFT_YELLOW;
  if (strcmp(q, "NO DATA") == 0) return TFT_ORANGE;
  return TFT_RED;
}
