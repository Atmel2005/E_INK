/**
 * SSD1681 Demo 08: Border Waveform Control
 * Different border waveform settings
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
int currentBorder = 0;

void drawBorder(int16_t margin, EPDColor color) {
  int16_t w = epd->width();
  int16_t h = epd->height();
  
  for (int x = margin; x < w - margin; x++) {
    epd->drawPixel(x, margin, color);
    epd->drawPixel(x, h - margin - 1, color);
  }
  for (int y = margin; y < h - margin; y++) {
    epd->drawPixel(margin, y, color);
    epd->drawPixel(w - margin - 1, y, color);
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
  const char* names[] = {"LUT0", "LUT1", "LUT2", "LUT3"};
  
  epd->fillScreen(EPDColor::White);
  
  // Set border waveform
  epd->setBorderWaveform(currentBorder);
  Serial.print("Border: ");
  Serial.println(names[currentBorder]);
  
  // Draw content
  drawBorder(5, EPDColor::Black);
  drawBorder(10, EPDColor::Black);
  
  // Pattern based on border setting
  for (int y = 20; y < 180; y++) {
    for (int x = 20; x < 180; x++) {
      if ((x + y + currentBorder * 15) % 30 < 15) {
        epd->drawPixel(x, y, EPDColor::Black);
      }
    }
  }
  
  // Border selector
  for (int i = 0; i < 4; i++) {
    int bx = 30 + i * 40;
    int by = 80;
    
    // Box
    for (int x = 0; x < 30; x++) {
      epd->drawPixel(bx + x, by, EPDColor::Black);
      epd->drawPixel(bx + x, by + 30, EPDColor::Black);
    }
    for (int y = 0; y < 30; y++) {
      epd->drawPixel(bx, by + y, EPDColor::Black);
      epd->drawPixel(bx + 29, by + y, EPDColor::Black);
    }
    
    // Highlight current
    if (i == currentBorder) {
      for (int x = 1; x < 29; x++) {
        for (int y = 1; y < 29; y++) {
          epd->drawPixel(bx + x, by + y, EPDColor::Black);
        }
      }
    }
  }
  
  epd->flushFull();
  
  currentBorder = (currentBorder + 1) % 4;
  delay(4000);
}
