// ST7789 Dsiplay Testing on Tenstar Esp32S3 

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_BMP280.h>

#include "ImuDrv.hpp"   // Newer SensorLib QMI8658 driver
// Donwload here: https://github.com/lewisxhe/SensorLib/tree/master

// -------------------- TFT DISPLAY PINS --------------------

#define TFT_CS         7
#define TFT_DC         39
#define TFT_RST        40
#define TFT_BACKLIGHT  45

// -------------------- SPI PINS FOR TFT --------------------

#define SPI_SCK   36
#define SPI_MISO  37
#define SPI_MOSI  35

// Adafruit Object for using methods 
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define PURPLE 0x780F

void startDisplay(){
  pinMode(TFT_BACKLIGHT, HIGH);

  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(5);
  Serial.println("Display started");
}

void drawName(){
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(PURPLE);
  char* name = "Niki";
  tft.setCursor(120, 67);
  tft.print(name);
}

void setup(){
  Serial.begin(115200);
  delay(1500);
  Serial.println("Esp Serial Ok");

  startDisplay();

  tft.setCursor(0, 0);
  tft.println("Starting");
}

void loop() {

  startDisplay();
  drawName();
}
