/**
 * SSD1681 Demo 05: Deep Sleep Modes
 * Power saving with two deep sleep modes
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

void drawPowerMeter(int16_t x, int16_t y, uint8_t level, const char* label) {
  // Battery shape
  for (int i = 0; i < 60; i++) {
    epd->drawPixel(x + i, y, EPDColor::Black);
    epd->drawPixel(x + i, y + 30, EPDColor::Black);
  }
  for (int i = 0; i < 30; i++) {
    epd->drawPixel(x, y + i, EPDColor::Black);
    epd->drawPixel(x + 59, y + i, EPDColor::Black);
  }
  for (int i = 0; i < 10; i++) {
    epd->drawPixel(x + 60, y + 10 + i, EPDColor::Black);
    epd->drawPixel(x + 64, y + 10 + i, EPDColor::Black);
  }
  
  // Fill level
  int fill = level * 56 / 100;
  for (int i = 2; i < fill + 2; i++) {
    for (int j = 2; j < 28; j++) {
      epd->drawPixel(x + i, y + j, EPDColor::Black);
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
  // Active mode
  epd->fillScreen(EPDColor::White);
  drawPowerMeter(70, 20, 100, "ACTIVE");
  for (int i = 0; i < 60; i++) epd->drawPixel(70 + i, 70, EPDColor::Black);
  epd->flushFull();
  Serial.println("Active mode - 3s");
  delay(3000);
  
  // Deep Sleep Mode 1
  epd->fillScreen(EPDColor::White);
  drawPowerMeter(70, 20, 75, "SLEEP1");
  for (int i = 0; i < 60; i++) epd->drawPixel(70 + i, 70, EPDColor::Black);
  epd->flushFull();
  Serial.println("Deep Sleep Mode 1");
  epd->enterDeepSleepMode1();
  delay(5000);
  epd->begin();
  
  // Deep Sleep Mode 2
  epd->fillScreen(EPDColor::White);
  drawPowerMeter(70, 20, 50, "SLEEP2");
  for (int i = 0; i < 60; i++) epd->drawPixel(70 + i, 70, EPDColor::Black);
  epd->flushFull();
  Serial.println("Deep Sleep Mode 2");
  epd->enterDeepSleepMode2();
  delay(5000);
  epd->begin();
  
  // Normal sleep
  epd->fillScreen(EPDColor::White);
  drawPowerMeter(70, 20, 25, "DEEP");
  for (int i = 0; i < 60; i++) epd->drawPixel(70 + i, 70, EPDColor::Black);
  epd->flushFull();
  Serial.println("Normal deep sleep");
  epd->sleep();
  delay(5000);
  epd->begin();
}
