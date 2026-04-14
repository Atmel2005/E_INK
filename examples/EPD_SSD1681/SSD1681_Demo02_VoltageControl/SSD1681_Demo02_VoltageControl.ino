/**
 * SSD1681 Demo 02: Voltage Control
 * Demonstrates Gate, Source, and VCOM voltage adjustment
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

void drawGauge(int16_t x, int16_t y, int16_t radius, uint8_t value, uint8_t max) {
  // Background arc
  for (int angle = 135; angle <= 405; angle += 3) {
    float rad = angle * 3.14159 / 180;
    int px = x + cos(rad) * radius;
    int py = y + sin(rad) * radius;
    epd->drawPixel(px, py, EPDColor::Black);
  }
  
  // Fill arc
  int fillAngle = 135 + (value * 270 / max);
  for (int angle = 135; angle <= fillAngle; angle += 3) {
    float rad = angle * 3.14159 / 180;
    for (int r = radius - 5; r <= radius; r++) {
      int px = x + cos(rad) * r;
      int py = y + sin(rad) * r;
      epd->drawPixel(px, py, EPDColor::Black);
    }
  }
  
  // Needle
  float needleRad = fillAngle * 3.14159 / 180;
  for (int r = 5; r < radius - 8; r++) {
    int px = x + cos(needleRad) * r;
    int py = y + sin(needleRad) * r;
    epd->drawPixel(px, py, EPDColor::Black);
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
  uint8_t vghLevels[] = {0, 10, 20, 30};
  uint8_t vcomLevels[] = {0, 50, 100, 150, 200};
  
  for (int cycle = 0; cycle < 5; cycle++) {
    epd->fillScreen(EPDColor::White);
    
    // Set voltages
    epd->setGateVoltage(vghLevels[cycle % 4]);
    epd->setVCOM(vcomLevels[cycle]);
    epd->setSourceVoltage(0x41, 0xA8, 0x32);
    
    // Draw gauges
    drawGauge(50, 60, 35, vghLevels[cycle % 4], 31);
    drawGauge(150, 60, 35, vcomLevels[cycle], 255);
    drawGauge(100, 140, 35, 128, 255);
    
    // Labels
    for (int i = 0; i < 30; i++) {
      epd->drawPixel(35 + i, 105, EPDColor::Black);
      epd->drawPixel(135 + i, 105, EPDColor::Black);
      epd->drawPixel(85 + i, 185, EPDColor::Black);
    }
    
    // Read back VCOM
    uint8_t vcom = epd->readVCOM();
    Serial.print("VCOM read: 0x");
    Serial.println(vcom, HEX);
    
    epd->flushFull();
    delay(4000);
  }
}
