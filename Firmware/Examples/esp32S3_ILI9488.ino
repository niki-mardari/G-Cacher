/*
  ESP32-S3 ILI9488 + XPT2046 Touch + SD Card Simple Test

  This sketch tests:
  - ILI9488 display
  - XPT2046 touch
  - SD card

  Required library:
  - LovyanGFX

  Serial Monitor:
  - 115200 baud
*/

#define LGFX_USE_V1

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <SPI.h>
#include <FS.h>
#include <SD.h>

// ------------------------------------------------------------
// Pin mapping
// ------------------------------------------------------------

// ILI9488 display SPI
#define TFT_CS    8
#define TFT_RST   14
#define TFT_DC    15
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_MISO  13

// XPT2046 touch chip select
#define TOUCH_CS  10
#define TOUCH_IRQ -1

// SD card dedicated SPI
#define SD_CS    9
#define SD_SCLK  36
#define SD_MOSI  35
#define SD_MISO  37

// Display size in portrait
#define SCREEN_W 320
#define SCREEN_H 480

// ------------------------------------------------------------
// LovyanGFX display + touch config
// ------------------------------------------------------------

class LGFX_ILI9488_Touch : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488 _panel;
  lgfx::Bus_SPI _bus;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX_ILI9488_Touch(void)
  {
    {
      auto cfg = _bus.config();

      cfg.spi_host = SPI3_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read = 6000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      cfg.pin_sclk = TFT_SCLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_dc   = TFT_DC;

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();

      cfg.pin_cs   = TFT_CS;
      cfg.pin_rst  = TFT_RST;
      cfg.pin_busy = -1;

      cfg.panel_width  = 320;
      cfg.panel_height = 480;
      cfg.memory_width  = 320;
      cfg.memory_height = 480;

      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;

      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;

      cfg.readable = false;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;

      _panel.config(cfg);
    }

    {
      auto cfg = _touch.config();

      cfg.spi_host = SPI3_HOST;
      cfg.freq = 1000000;

      cfg.pin_sclk = TFT_SCLK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_cs   = TOUCH_CS;
      cfg.pin_int  = TOUCH_IRQ;

      cfg.bus_shared = true;

      // Basic calibration values.
      // If touch is mirrored, swap x_min/x_max or y_min/y_max.
      cfg.x_min = 200;
      cfg.x_max = 3900;
      cfg.y_min = 200;
      cfg.y_max = 3900;

      cfg.offset_rotation = 0;

      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};

LGFX_ILI9488_Touch tft;

// SD uses its own SPI bus
SPIClass sdSPI(FSPI);

bool sdOK = false;
String sdStatus = "NOT TESTED";

uint32_t touchCount = 0;
uint32_t lastStatusUpdate = 0;

// ------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------

void drawHeader()
{
  tft.fillRect(0, 0, SCREEN_W, 50, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(2);
  tft.setCursor(14, 14);
  tft.print("ILI9488 TEST");
}

void drawStaticScreen()
{
  tft.fillScreen(TFT_BLACK);
  drawHeader();

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(20, 70);
  tft.print("Display: OK");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 110);
  tft.print("Touch the screen.");

  tft.setCursor(20, 145);
  tft.print("SD test result:");

  tft.drawRoundRect(15, 180, 290, 100, 8, TFT_DARKGREY);
  tft.drawRoundRect(15, 305, 290, 90, 8, TFT_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(20, 430);
  tft.print("Only display + touch + SD are being tested.");
}

const char *cardTypeName(uint8_t type)
{
  if (type == CARD_MMC) return "MMC";
  if (type == CARD_SD) return "SDSC";
  if (type == CARD_SDHC) return "SDHC";
  return "UNKNOWN";
}

bool tryStartSD(uint32_t freq)
{
  SD.end();
  delay(50);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  delay(5);

  Serial.print("Trying SD at ");
  Serial.print(freq);
  Serial.println(" Hz");

  bool ok = SD.begin(SD_CS, sdSPI, freq, "/sd", 5);
  digitalWrite(SD_CS, HIGH);

  return ok;
}

bool startSD()
{
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  if (tryStartSD(400000)) return true;
  if (tryStartSD(1000000)) return true;
  if (tryStartSD(4000000)) return true;

  return false;
}

void testSDFile()
{
  if (!sdOK)
  {
    sdStatus = "FAILED";
    return;
  }

  File file = SD.open("/touch_sd_test.txt", FILE_APPEND);

  if (!file)
  {
    sdStatus = "OPEN FAILED";
    return;
  }

  file.print("Boot test at millis=");
  file.println(millis());
  file.close();

  File readFile = SD.open("/touch_sd_test.txt", FILE_READ);

  if (!readFile)
  {
    sdStatus = "READ FAILED";
    return;
  }

  readFile.close();

  sdStatus = "OK";
}

void drawSDStatus()
{
  tft.fillRect(20, 205, 270, 55, TFT_BLACK);

  tft.setTextSize(2);
  tft.setCursor(25, 205);

  if (sdOK)
  {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("SD: OK");

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(25, 235);
    tft.print("Type: ");
    tft.print(cardTypeName(SD.cardType()));
  }
  else
  {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("SD: FAILED");
  }
}

void appendTouchToSD(uint16_t x, uint16_t y)
{
  if (!sdOK) return;

  File file = SD.open("/touch_log.csv", FILE_APPEND);

  if (!file)
  {
    Serial.println("Could not open /touch_log.csv");
    return;
  }

  file.print(millis());
  file.print(",");
  file.print(x);
  file.print(",");
  file.println(y);
  file.close();
}

void drawTouchInfo(uint16_t x, uint16_t y)
{
  tft.fillRect(20, 330, 270, 45, TFT_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(25, 330);
  tft.print("Touch #");
  tft.print(touchCount);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(25, 360);
  tft.print("X:");
  tft.print(x);
  tft.print("  Y:");
  tft.print(y);

  // Small marker where you touched
  tft.fillCircle(x, y, 5, TFT_RED);
}

void handleTouch()
{
  static bool wasTouching = false;
  static uint32_t lastTouchTime = 0;

  uint16_t x = 0;
  uint16_t y = 0;

  bool touching = tft.getTouch(&x, &y);

  if (touching && (!wasTouching || millis() - lastTouchTime > 250))
  {
    lastTouchTime = millis();
    touchCount++;

    Serial.print("Touch #");
    Serial.print(touchCount);
    Serial.print("  X=");
    Serial.print(x);
    Serial.print("  Y=");
    Serial.println(y);

    drawTouchInfo(x, y);
    appendTouchToSD(x, y);
  }

  wasTouching = touching;
}

void updateStatusSmallArea()
{
  if (millis() - lastStatusUpdate < 1000) return;
  lastStatusUpdate = millis();

  tft.fillRect(20, 400, 280, 22, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(20, 405);
  tft.print("Uptime: ");
  tft.print(millis() / 1000);
  tft.print("s   Touches: ");
  tft.print(touchCount);
}

// ------------------------------------------------------------
// Setup and loop
// ------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("ILI9488 + Touch + SD Simple Test");
  Serial.println("ESP32-S3");
  Serial.println("=================================");

  pinMode(TFT_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  tft.init();
  tft.setRotation(2);
  tft.setBrightness(255);

  drawStaticScreen();

  Serial.println("Display initialized.");

  sdOK = startSD();
  testSDFile();

  Serial.print("SD status: ");
  Serial.println(sdStatus);

  drawSDStatus();

  Serial.println("Touch the screen to test touch coordinates.");
  Serial.println("Touch points will also be saved to /touch_log.csv if SD is OK.");
}

void loop()
{
  handleTouch();
  updateStatusSmallArea();
}