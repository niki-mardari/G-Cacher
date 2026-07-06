/********************************************************
  GNSS Sat Cacher - Clean Modular Firmware
  Tenstar ESP32-S3 + LC29H GNSS + ILI9488/XPT2046 + SD + Backend

  Current focus:
  - Same core project features as the original sketch
  - Cleaner file structure
  - Less screen flicker by updating only changed fields
  - Less network blocking by using slower retry/backoff timing
  - No hardcoded private Wi-Fi/server details
********************************************************/

#define LGFX_USE_V1

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "AppState.h"
#include "Buzzer.h"
#include "Config.h"
#include "GnssManager.h"
#include "Hardware.h"
#include "InputManager.h"
#include "NetworkManager.h"
#include "SensorManager.h"
#include "StorageManager.h"
#include "UiManager.h"

AppState app;

void setup()
{
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("====================================");
  Serial.println("GNSS Sat Cacher powered on");
  Serial.println("====================================");

  initHardwarePins();
  initBuzzer();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  initDisplay(app);
  initSensors(app);
  initGnss(app);
  initStorage(app);
  initNetwork(app);

  drawCurrentScreen(app);
  playStartupMelody();

  Serial.println("Setup complete");
}

void loop()
{
  handleBootButton(app);
  updateTouch(app);

  updateGnss(app);
  updateSensors(app);
  updateNetwork(app);
  updateStorage(app);

  updateBuzzer();
  updateUi(app);
}
