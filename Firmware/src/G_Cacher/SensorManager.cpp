#include "SensorManager.h"
#include "Config.h"
#include "Hardware.h"
#include <Wire.h>
#include <math.h>

void initSensors(AppState &app)
{
  if (bmp.begin(BMP_ADDR_1))
  {
    app.bmpOK = true;
    Serial.println("BMP280 found at 0x77");
  }
  else if (bmp.begin(BMP_ADDR_2))
  {
    app.bmpOK = true;
    Serial.println("BMP280 found at 0x76");
  }
  else
  {
    app.bmpOK = false;
    Serial.println("BMP280 not found");
  }

  if (qmi.begin(Wire, QMI_ADDR_1, I2C_SDA, I2C_SCL))
  {
    app.qmiOK = true;
    Serial.println("QMI8658C found at 0x6B");
  }
  else if (qmi.begin(Wire, QMI_ADDR_2, I2C_SDA, I2C_SCL))
  {
    app.qmiOK = true;
    Serial.println("QMI8658C found at 0x6A");
  }
  else
  {
    app.qmiOK = false;
    Serial.println("QMI8658C not found");
    return;
  }

  qmi.configAccel(AccelFullScaleRange::FS_8G, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
  qmi.configGyro(GyroFullScaleRange::FS_1000_DPS, 1000.0f, SensorQMI8658::LpfMode::MODE_0);
  qmi.enableAccel();
  qmi.enableGyro();
}

void updateSensors(AppState &app)
{
  if (millis() - app.lastSensorUpdate < 150) return;
  app.lastSensorUpdate = millis();

  if (app.bmpOK)
  {
    app.bmpTempC = bmp.readTemperature();
    app.bmpPressureHpa = bmp.readPressure() / 100.0f;
  }

  if (app.qmiOK && qmi.isDataReady(static_cast<uint8_t>(ImuBase::DataReadyMask::ACCEL)))
  {
    qmi.readAccel(acc);
    qmi.readGyro(gyr);
    app.accX = acc.mps2.x;
    app.accY = acc.mps2.y;
    app.accZ = acc.mps2.z;
    app.accMagnitude = sqrt((app.accX * app.accX) + (app.accY * app.accY) + (app.accZ * app.accZ));
  }
}

const char *thermalStatus(const AppState &app)
{
  if (!app.bmpOK) return "NO BMP";
  if (app.bmpTempC < 50.0f) return "OK";
  if (app.bmpTempC < 60.0f) return "WARM";
  return "HOT";
}

uint16_t thermalColour(const AppState &app)
{
  if (!app.bmpOK) return TFT_RED;
  if (app.bmpTempC < 50.0f) return TFT_GREEN;
  if (app.bmpTempC < 60.0f) return TFT_YELLOW;
  return TFT_RED;
}
