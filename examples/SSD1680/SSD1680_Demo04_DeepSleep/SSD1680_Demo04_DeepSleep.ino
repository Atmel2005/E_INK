/**
 * SSD1680 Demo 04: Deep Sleep Modes
 * Demonstrates two deep sleep modes for power saving
 */

#include <SPI.h>
#include <EPD_Base.h>

#define EPD_CONTROLLER EPDController::SSD1680

#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7, PIN_SCLK = 6, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 6, PIN_SCLK = 4, PIN_CS = 7, PIN_DC = 5, PIN_RST = 2, PIN_BUSY = 3;
#else
  const int PIN_MOSI = 11, PIN_SCLK = 13, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 7;
#endif

EPDConfig cfg;
EPD_Base* epd = nullptr;

void drawBattery(int16_t x, int16_t y, uint8_t level) {
  // Battery outline
  for (int i = 0; i < 30; i++) {
    epd->drawPixel(x + i, y, EPDColor::Black);
    epd->drawPixel(x + i, y + 14, EPDColor::Black);
  }
  for (int i = 0; i < 15; i++) {
    epd->drawPixel(x, y + i, EPDColor::Black);
    epd->drawPixel(x + 29, y + i, EPDColor::Black);
  }
  // Battery tip
  for (int i = 0; i < 6; i++) {
    epd->drawPixel(x + 30, y + 4 + i, EPDColor::Black);
    epd->drawPixel(x + 32, y + 4 + i, EPDColor::Black);
  }
  // Fill level
  int fill = level * 26 / 100;
  for (int i = 2; i < fill + 2; i++) {
    for (int j = 2; j < 13; j++) {
      epd->drawPixel(x + i, y + j, EPDColor::Black);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  cfg.width = 122;
  cfg.height = 250;
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
  
  epd->setRotation(1);
}

void loop() {
  // Display active mode
  epd->fillScreen(EPDColor::White);
  
  // Draw "ACTIVE" text
  for (int i = 0; i < 50; i++) {
    epd->drawPixel(10 + i, 20, EPDColor::Black);
  }
  
  drawBattery(10, 40, 100);
  
  epd->flushFull();
  Serial.println("Active mode - waiting 5s");
  delay(5000);
  
  // Deep Sleep Mode 1
  epd->fillScreen(EPDColor::White);
  for (int i = 0; i < 50; i++) {
    epd->drawPixel(10 + i, 20, EPDColor::Black);
  }
  drawBattery(10, 40, 75);
  
  epd->flushFull();
  Serial.println("Entering Deep Sleep Mode 1");
  epd->enterDeepSleepMode1();
  
  delay(10000); // Sleep for 10 seconds
  
  // Wake up requires hardware reset
  epd->begin(); // Re-initialize
  
  // Deep Sleep Mode 2 (lower power)
  epd->fillScreen(EPDColor::White);
  for (int i = 0; i < 50; i++) {
    epd->drawPixel(10 + i, 20, EPDColor::Black);
  }
  drawBattery(10, 40, 50);
  
  epd->flushFull();
  Serial.println("Entering Deep Sleep Mode 2");
  epd->enterDeepSleepMode2();
  
  delay(10000);
  
  epd->begin();
  
  // Normal sleep (legacy method)
  epd->fillScreen(EPDColor::White);
  for (int i = 0; i < 50; i++) {
    epd->drawPixel(10 + i, 20, EPDColor::Black);
  }
  drawBattery(10, 40, 25);
  
  epd->flushFull();
  Serial.println("Normal sleep mode");
  epd->sleep();
  
  delay(10000);
  
  epd->begin();
}
