/**
 * SSD1681 Demo 04: Color Display (BW_R)
 * Demonstrates black/white/red display capabilities
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

void drawColorWheel(int16_t cx, int16_t cy, int16_t radius) {
  for (int angle = 0; angle < 360; angle++) {
    float rad = angle * 3.14159 / 180;
    int x = cx + cos(rad) * radius;
    int y = cy + sin(rad) * radius;
    
    EPDColor color;
    if (angle < 120) color = EPDColor::Black;
    else if (angle < 240) color = EPDColor::Red;
    else color = EPDColor::White;
    
    for (int r = radius - 20; r <= radius; r++) {
      int px = cx + cos(rad) * r;
      int py = cy + sin(rad) * r;
      if (color != EPDColor::White) {
        epd->drawPixel(px, py, color);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  cfg.width = 200;
  cfg.height = 200;
  cfg.variant = EPDVariant::BW_R;  // Black/White/Red display
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
  
  // Draw color wheel
  drawColorWheel(100, 100, 70);
  
  // Draw colored rectangles
  for (int i = 0; i < 30; i++) {
    epd->drawPixel(10 + i, 10, EPDColor::Black);
    epd->drawPixel(10 + i, 40, EPDColor::Black);
  }
  for (int i = 0; i < 30; i++) {
    epd->drawPixel(10, 10 + i, EPDColor::Black);
    epd->drawPixel(39, 10 + i, EPDColor::Black);
  }
  
  // Red rectangle
  for (int x = 50; x < 80; x++) {
    for (int y = 10; y < 40; y++) {
      epd->drawPixel(x, y, EPDColor::Red);
    }
  }
  
  // Black rectangle
  for (int x = 90; x < 120; x++) {
    for (int y = 10; y < 40; y++) {
      epd->drawPixel(x, y, EPDColor::Black);
    }
  }
  
  // Pattern at bottom
  for (int x = 10; x < 190; x++) {
    for (int y = 170; y < 190; y++) {
      if ((x + y) % 20 < 10) {
        epd->drawPixel(x, y, EPDColor::Black);
      } else if ((x + y) % 30 < 15) {
        epd->drawPixel(x, y, EPDColor::Red);
      }
    }
  }
  
  epd->flushFull();
  delay(10000);
  
  // Partial update demo
  for (int frame = 0; frame < 5; frame++) {
    for (int y = 50; y < 150; y++) {
      for (int x = 20; x < 180; x++) {
        epd->drawPixel(x, y, EPDColor::White);
      }
    }
    
    // Draw frame number
    for (int i = 0; i < 50; i++) {
      for (int j = 0; j < 50; j++) {
        if ((i + j + frame * 10) % 20 < 10) {
          epd->drawPixel(75 + i, 75 + j, EPDColor::Red);
        }
      }
    }
    
    epd->flushRect(20, 50, 160, 100);
    delay(2000);
  }
}
