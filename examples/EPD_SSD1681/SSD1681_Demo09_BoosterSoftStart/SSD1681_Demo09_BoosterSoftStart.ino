/**
 * SSD1681 Demo 09: Booster Soft Start
 * Soft start configuration for inrush current reduction
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

void drawPhaseBar(int16_t x, int16_t y, uint8_t value, int phase) {
  // Bar outline
  for (int i = 0; i < 50; i++) {
    epd->drawPixel(x + i, y, EPDColor::Black);
    epd->drawPixel(x + i, y + 20, EPDColor::Black);
  }
  for (int i = 0; i < 20; i++) {
    epd->drawPixel(x, y + i, EPDColor::Black);
    epd->drawPixel(x + 49, y + i, EPDColor::Black);
  }
  
  // Fill
  int fill = value * 46 / 255;
  for (int i = 2; i < fill + 2; i++) {
    for (int j = 2; j < 18; j++) {
      epd->drawPixel(x + i, y + j, EPDColor::Black);
    }
  }
  
  // Phase indicator
  for (int i = 0; i < 10; i++) {
    epd->drawPixel(x + 20 + i, y + 25, EPDColor::Black);
  }
}

void drawWaveform(int16_t x, int16_t y, uint8_t a, uint8_t b, uint8_t c) {
  // Draw waveform visualization
  int levels[] = {a * 40 / 255, b * 60 / 255, c * 80 / 255};
  
  for (int phase = 0; phase < 3; phase++) {
    int px = x + phase * 50;
    int level = levels[phase];
    
    // Rising edge
    for (int i = 0; i < 40; i++) {
      int h = level * i / 40;
      for (int j = 0; j < h; j++) {
        epd->drawPixel(px + i, y + 80 - j, EPDColor::Black);
      }
    }
    
    // Level
    for (int i = 40; i < 50; i++) {
      for (int j = 0; j < level; j++) {
        epd->drawPixel(px + i, y + 80 - j, EPDColor::Black);
      }
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
  uint8_t configs[][3] = {
    {0x8B, 0x9C, 0x96},  // Default
    {0x00, 0x00, 0x00},  // Minimum
    {0xFF, 0xFF, 0xFF},  // Maximum
    {0x40, 0x80, 0xC0},  // Custom
  };
  const char* names[] = {"Default", "Minimum", "Maximum", "Custom"};
  
  for (int c = 0; c < 4; c++) {
    epd->fillScreen(EPDColor::White);
    
    epd->setBoosterSoftStart(configs[c][0], configs[c][1], configs[c][2]);
    epd->setGateLineWidth(0x0F);
    epd->setPWMFrequency(0x1F);
    
    Serial.print("Booster: ");
    Serial.print(names[c]);
    Serial.print(" (");
    Serial.print(configs[c][0], HEX);
    Serial.print(",");
    Serial.print(configs[c][1], HEX);
    Serial.print(",");
    Serial.print(configs[c][2], HEX);
    Serial.println(")");
    
    // Draw waveform
    drawWaveform(10, 30, configs[c][0], configs[c][1], configs[c][2]);
    
    // Draw phase bars
    drawPhaseBar(20, 130, configs[c][0], 1);
    drawPhaseBar(80, 130, configs[c][1], 2);
    drawPhaseBar(140, 130, configs[c][2], 3);
    
    epd->flushFull();
    delay(4000);
  }
}
