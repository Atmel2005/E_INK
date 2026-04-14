/**
 * SSD1681 Demo 03: Partial Update Modes
 * Fast partial refresh with counter display
 * Resolution: 200x200
 */

#include <SPI.h>
#include <EPD_Base.h>

#define EPD_CONTROLLER EPDController::SSD1681

#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7, PIN_SCLK = 6, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 6, PIN_SCLK = 4, PIN_CS = 7, PIN_DC = 5, PIN_RST = 2, PIN_BUSY = 3;
#else
  const int PIN_MOSI = 11, PIN_SCLK = 13, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 7;
#endif

EPDConfig cfg;
EPD_Base* epd = nullptr;
int counter = 0;

void drawLargeDigit(int16_t x, int16_t y, int digit) {
  const uint8_t segs[10] = {0x77, 0x24, 0x5D, 0x6D, 0x2E, 0x6B, 0x7B, 0x25, 0x7F, 0x6F};
  uint8_t s = segs[digit];
  
  int w = 20, h = 35, t = 4;
  
  if (s & 0x01) for (int i = t; i < w-t; i++) for (int j = 0; j < t; j++) epd->drawPixel(x + i, y + j, EPDColor::Black);
  if (s & 0x02) for (int i = 0; i < t; i++) for (int j = t; j < h; j++) epd->drawPixel(x + w + i, y + j, EPDColor::Black);
  if (s & 0x04) for (int i = 0; i < t; i++) for (int j = h+t; j < h*2; j++) epd->drawPixel(x + w + i, y + j, EPDColor::Black);
  if (s & 0x08) for (int i = t; i < w-t; i++) for (int j = 0; j < t; j++) epd->drawPixel(x + i, y + h*2 + j, EPDColor::Black);
  if (s & 0x10) for (int i = 0; i < t; i++) for (int j = h+t; j < h*2; j++) epd->drawPixel(x + i, y + j, EPDColor::Black);
  if (s & 0x20) for (int i = 0; i < t; i++) for (int j = t; j < h; j++) epd->drawPixel(x + i, y + j, EPDColor::Black);
  if (s & 0x40) for (int i = t; i < w-t; i++) for (int j = 0; j < t; j++) epd->drawPixel(x + i, y + h + j, EPDColor::Black);
}

void setup() {
  Serial.begin(115200);
  
  cfg.width = 200;
  cfg.height = 200;
  cfg.variant = EPDVariant::BW;
  cfg.controller = EPD_CONTROLLER;
  cfg.use_hw_spi = false;
  cfg.cs = PIN_CS; cfg.dc = PIN_DC; cfg.rst = PIN_RST;
  cfg.busy = PIN_BUSY; cfg.mosi = PIN_MOSI; cfg.sclk = PIN_SCLK;
  
  epd = createEPD(cfg);
  if (!epd || !epd->begin()) {
    Serial.println("EPD init failed!");
    while(1);
  }
  
  epd->setRotation(0);
  epd->setDisplayMode(DisplayMode::Mode2);
  epd->setRefreshProfile(RefreshProfile::Partial);
  
  epd->fillScreen(EPDColor::White);
  epd->flushFull();
}

void loop() {
  // Static frame
  for (int i = 0; i < 180; i++) {
    epd->drawPixel(10 + i, 10, EPDColor::Black);
    epd->drawPixel(10 + i, 180, EPDColor::Black);
    epd->drawPixel(10, 10 + i, EPDColor::Black);
    epd->drawPixel(190, 10 + i, EPDColor::Black);
  }
  
  // Clear counter area
  for (int y = 50; y < 150; y++) {
    for (int x = 20; x < 180; x++) {
      epd->drawPixel(x, y, EPDColor::White);
    }
  }
  
  // Draw counter
  int d0 = counter % 10;
  int d1 = (counter / 10) % 10;
  int d2 = (counter / 100) % 10;
  
  drawLargeDigit(30, 60, d2);
  drawLargeDigit(75, 60, d1);
  drawLargeDigit(120, 60, d0);
  
  // Colons
  for (int i = 0; i < 8; i++) {
    epd->drawPixel(68, 85 + i, EPDColor::Black);
    epd->drawPixel(68, 110 + i, EPDColor::Black);
    epd->drawPixel(113, 85 + i, EPDColor::Black);
    epd->drawPixel(113, 110 + i, EPDColor::Black);
  }
  
  epd->flushRect(20, 50, 160, 100);
  
  counter = (counter + 1) % 1000;
  
  // Full refresh every 100 updates
  if (counter % 100 == 0) {
    epd->fillScreen(EPDColor::White);
    epd->flushFull();
  }
  
  delay(300);
}
