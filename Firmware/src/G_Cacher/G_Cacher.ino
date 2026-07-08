/********************************************************
  GNSS Sat Cacher Firmware
  Last changed: 08/07/2026

  Hardware:
  Tenstar ESP32-S3 + LC29H GNSS + ILI9488/XPT2046 + SD + Backend

  What is done:
  - Cleaner file structure
  - Less screen flicker by updating only changed fields
  - Less network blocking by using slower retry/backoff timing
  - No hardcoded private Wi-Fi/server details

  Current issues: 
  - GNSS SOG (Speed over ground) unstable when device is still. 
  Happens due to GNSS noise and signal bouncing. 
  Need Kalman Filtering and sensor Fusion implementation with the QMI8658C for short term correction and GNSS for long term correction
  A simpler solution would be to ignore values lower than 1Km when displayed in UiManager currently implemented
  - Time Seconds delay, seconds don't update quicly enough/ not at the same pace. UI updates may be too heavy, Possibly implement internal timer for calculating seconds or
  move date time to home screen where not much gets updated. 
  
  
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
