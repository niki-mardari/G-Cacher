#include "Hardware.h"
#include "Config.h"
#include "Buzzer.h"

LGFX_ILI9488_Touch tft;
SPIClass sdSPI(FSPI);
HardwareSerial GNSS(1);
TinyGPSPlus gps;
Adafruit_BMP280 bmp;
SensorQMI8658 qmi;
AccelerometerData acc;
GyroscopeData gyr;

LGFX_ILI9488_Touch::LGFX_ILI9488_Touch()
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
    cfg.pin_sclk = BIG_TFT_SCLK;
    cfg.pin_mosi = BIG_TFT_MOSI;
    cfg.pin_miso = BIG_TFT_MISO;
    cfg.pin_dc = BIG_TFT_DC;
    _bus.config(cfg);
    _panel.setBus(&_bus);
  }

  {
    auto cfg = _panel.config();
    cfg.pin_cs = BIG_TFT_CS;
    cfg.pin_rst = BIG_TFT_RST;
    cfg.pin_busy = -1;
    cfg.panel_width = 320;
    cfg.panel_height = 480;
    cfg.memory_width = 320;
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
    cfg.pin_sclk = BIG_TFT_SCLK;
    cfg.pin_mosi = BIG_TFT_MOSI;
    cfg.pin_miso = BIG_TFT_MISO;
    cfg.pin_cs = TOUCH_CS;
    cfg.pin_int = TOUCH_IRQ;
    cfg.bus_shared = true;
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

void initHardwarePins()
{
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(BIG_TFT_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(BIG_TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
}

void initDisplay(AppState &app)
{
  bool ok = tft.init();
  Serial.print("Display init: ");
  Serial.println(ok ? "OK" : "FAILED");

  tft.setRotation(DISPLAY_ROTATION);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(32, 210);
  tft.print("GNSS Sat Cacher");
  app.forceRedraw = true;
}
