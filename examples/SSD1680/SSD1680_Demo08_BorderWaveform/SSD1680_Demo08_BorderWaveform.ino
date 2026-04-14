/**
 * SSD1680 Demo 08: Border Waveform Control
 * Demonstrates different border waveform settings
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

int currentBorder = 0;

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

void drawBorder(int16_t w, int16_t h, EPDColor color) {
  for (int x = 0; x < w; x++) {
    epd->drawPixel(x, 0, color);
    epd->drawPixel(x, h-1, color);
  }
  for (int y = 0; y < h; y++) {
    epd->drawPixel(0, y, color);
    epd->drawPixel(w-1, y, color);
  }
}

void loop() {
  const char* borderNames[] = {"LUT0", "LUT1", "LUT2", "LUT3"};
  
  epd->fillScreen(EPDColor::White);
  
  // Set border waveform
  epd->setBorderWaveform(currentBorder);
  Serial.print("Border waveform: ");
  Serial.println(borderNames[currentBorder]);
  
  // Draw content area
  int16_t margin = 5;
  int16_t innerW = epd->width() - margin * 2;
  int16_t innerH = epd->height() - margin * 2;
  
  // Inner border
  drawBorder(innerW, innerH, EPDColor::Black);
  
  // Content
  for (int y = margin + 5; y < epd->height() - margin - 5; y++) {
    for (int x = margin + 5; x < epd->width() - margin - 5; x++) {
      // Pattern based on border setting
      if ((x + y + currentBorder * 10) % 20 < 10) {
        epd->drawPixel(x, y, EPDColor::Black);
      }
    }
  }
  
  // Border setting indicator
  for (int i = 0; i < 4; i++) {
    int16_t bx = 20 + i * 25;
    int16_t by = epd->height() / 2;
    
    // Box
    for (int x = 0; x < 20; x++) {
      epd->drawPixel(bx + x, by, EPDColor::Black);
      epd->drawPixel(bx + x, by + 20, EPDColor::Black);
    }
    for (int y = 0; y < 20; y++) {
      epd->drawPixel(bx, by + y, EPDColor::Black);
      epd->drawPixel(bx + 19, by + y, EPDColor::Black);
    }
    
    // Fill current
    if (i == currentBorder) {
      for (int x = 1; x < 19; x++) {
        for (int y = 1; y < 19; y++) {
          epd->drawPixel(bx + x, by + y, EPDColor::Black);
        }
      }
    }
  }
  
  epd->flushFull();
  
  currentBorder = (currentBorder + 1) % 4;
  delay(5000);
}
