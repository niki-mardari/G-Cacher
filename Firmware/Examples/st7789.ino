// ST7789 Display Testing on Tenstar ESP32-S3

#include <Arduino.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// -------------------- TFT DISPLAY PINS --------------------

#define TFT_CS         7
#define TFT_DC         39
#define TFT_RST        40
#define TFT_BACKLIGHT  45

// -------------------- SPI PINS FOR TFT --------------------

#define SPI_SCK   36
#define SPI_MISO  37
#define SPI_MOSI  35

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define PURPLE 0x780F

void startDisplay() {
  Serial.println("Starting display...");

  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, TFT_CS);

  tft.init(135, 240);
  tft.setRotation(3);

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  Serial.println("Display started");
}

void drawName() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(PURPLE);
  tft.setTextSize(5);

  const char *name = "G-Cacher";

  // For 135x240 display with rotation 3:
  // screen becomes 240 wide x 135 height.
  tft.setCursor(0, 50);
  tft.print(name);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("ESP Serial OK");

  startDisplay();

  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Starting");

  delay(1000);

  drawName();
}

void loop() {
  // Nothing here :)
}