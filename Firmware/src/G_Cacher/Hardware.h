#pragma once

#ifndef LGFX_USE_V1
#define LGFX_USE_V1
#endif

#include <Arduino.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include <Adafruit_BMP280.h>
#include <LovyanGFX.hpp>
#include "ImuDrv.hpp"
#include "AppState.h"

class LGFX_ILI9488_Touch : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX_ILI9488_Touch();
};

extern LGFX_ILI9488_Touch tft;
extern SPIClass sdSPI;
extern HardwareSerial GNSS;
extern TinyGPSPlus gps;
extern Adafruit_BMP280 bmp;
extern SensorQMI8658 qmi;
extern AccelerometerData acc;
extern GyroscopeData gyr;

void initHardwarePins();
void initDisplay(AppState &app);
