/**
 * SSD1680 Demo 09: Booster Soft Start
 * Demonstrates soft start configuration for inrush current reduction
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

void drawWaveform(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t phase) {
  // Draw soft start waveform visualization
  int step = w / 3;
  
  // Phase 1
  for (int i = 0; i < step; i++) {
    int val = (phase & 0x01) ? h/3 : h/4;
    for (int j = 0; j < val; j++) {
      epd->drawPixel(x + i, y + h - j, EPDColor::Black);
    }
  }
  
  // Phase 2
  for (int i = 0; i < step; i++) {
    int val = (phase & 0x02) ? h*2/3 : h/2;
    for (int j = 0; j < val; j++) {
      epd->drawPixel(x + step + i, y + h - j, EPDColor::Black);
    }
  }
  
  // Phase 3
  for (int i = 0; i < step; i++) {
    int val = (phase & 0x04) ? h : h*2/3;
    for (int j = 0; j < val; j++) {
      epd->drawPixel(x + step*2 + i, y + h - j, EPDColor::Black);
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
  // Test different soft start configurations
  uint8_t configs[][3] = {
    {0x8B, 0x9C, 0x96},  // Default (POR)
    {0x00, 0x00, 0x00},  // Minimum soft start
    {0xFF, 0xFF, 0xFF},  // Maximum soft start
    {0x40, 0x80, 0xC0},  // Custom
  };
  
  const char* names[] = {"Default", "Minimum", "Maximum", "Custom"};
  
  for (int c = 0; c < 4; c++) {
    epd->fillScreen(EPDColor::White);
    
    // Set booster soft start
    epd->setBoosterSoftStart(configs[c][0], configs[c][1], configs[c][2]);
    
    Serial.print("Booster config: ");
    Serial.print(names[c]);
    Serial.print(" (0x");
    Serial.print(configs[c][0], HEX);
    Serial.print(", 0x");
    Serial.print(configs[c][1], HEX);
    Serial.print(", 0x");
    Serial.print(configs[c][2], HEX);
    Serial.println(")");
    
    // Draw waveform visualization
    drawWaveform(10, 20, 100, 50, c);
    
    // Draw phase indicators
    for (int p = 0; p < 3; p++) {
      int16_t px = 10 + p * 35;
      int16_t py = 80;
      
      // Phase box
      for (int i = 0; i < 30; i++) {
        epd->drawPixel(px + i, py, EPDColor::Black);
        epd->drawPixel(px + i, py + 30, EPDColor::Black);
      }
      for (int i = 0; i < 30; i++) {
        epd->drawPixel(px, py + i, EPDColor::Black);
        epd->drawPixel(px + 29, py + i, EPDColor::Black);
      }
      
      // Fill based on config
      int fill = configs[c][p] * 28 / 255;
      for (int i = 1; i < fill; i++) {
        for (int j = 1; j < 29; j++) {
          epd->drawPixel(px + i, py + j, EPDColor::Black);
        }
      }
    }
    
    // Set gate line width for comparison
    epd->setGateLineWidth(0x0F);
    
    // Set PWM frequency
    epd->setPWMFrequency(0x1F);
    
    epd->flushFull();
    delay(5000);
  }
}
