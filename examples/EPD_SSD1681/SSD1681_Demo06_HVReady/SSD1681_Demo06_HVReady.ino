/**
 * SSD1681 Demo 06: HV Ready Detection
 * Analog block ready detection and VCOM sensing
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

void drawStatusIndicator(int16_t x, int16_t y, bool ready) {
  // Circle
  for (int angle = 0; angle < 360; angle += 5) {
    float rad = angle * 3.14159 / 180;
    int px = x + cos(rad) * 30;
    int py = y + sin(rad) * 30;
    epd->drawPixel(px, py, EPDColor::Black);
  }
  
  if (ready) {
    // Checkmark
    for (int i = 0; i < 20; i++) {
      epd->drawPixel(x - 10 + i, y + i - 5, EPDColor::Black);
      if (i < 15) epd->drawPixel(x - 10 + i, y - i + 10, EPDColor::Black);
    }
  } else {
    // X mark
    for (int i = 0; i < 30; i++) {
      epd->drawPixel(x - 15 + i, y - 15 + i, EPDColor::Black);
      epd->drawPixel(x + 15 - i, y - 15 + i, EPDColor::Black);
    }
  }
}

void drawVCOMMeter(int16_t x, int16_t y, uint8_t value) {
  // Meter background
  for (int i = 0; i < 100; i++) {
    epd->drawPixel(x + i, y, EPDColor::Black);
    epd->drawPixel(x + i, y + 20, EPDColor::Black);
  }
  for (int i = 0; i < 20; i++) {
    epd->drawPixel(x, y + i, EPDColor::Black);
    epd->drawPixel(x + 99, y + i, EPDColor::Black);
  }
  
  // Fill
  int fill = value * 96 / 255;
  for (int i = 2; i < fill + 2; i++) {
    for (int j = 2; j < 18; j++) {
      epd->drawPixel(x + i, y + j, EPDColor::Black);
    }
  }
  
  // Tick marks
  for (int i = 0; i < 5; i++) {
    int tx = x + i * 24;
    for (int j = 0; j < 5; j++) {
      epd->drawPixel(tx, y + 20 + j, EPDColor::Black);
    }
  }
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
}

void loop() {
  epd->fillScreen(EPDColor::White);
  
  // Check HV Ready
  bool hvReady = epd->checkHVReady();
  Serial.print("HV Ready: ");
  Serial.println(hvReady ? "YES" : "NO");
  
  // Draw HV status
  drawStatusIndicator(60, 60, hvReady);
  
  // Perform VCOM Sense
  uint8_t vcom = epd->performVCOMSense();
  Serial.print("VCOM Sense: 0x");
  Serial.println(vcom, HEX);
  
  // Draw VCOM meter
  drawVCOMMeter(50, 120, vcom);
  
  // Labels
  for (int i = 0; i < 40; i++) {
    epd->drawPixel(40 + i, 100, EPDColor::Black);
  }
  for (int i = 0; i < 60; i++) {
    epd->drawPixel(70 + i, 150, EPDColor::Black);
  }
  
  epd->flushFull();
  delay(10000);
}
