/**
 * SSD1680 Demo 05: HV Ready Detection
 * Demonstrates analog block ready detection
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
  epd->fillScreen(EPDColor::White);
  
  // Check HV Ready
  Serial.println("Checking HV Ready...");
  bool hvReady = epd->checkHVReady();
  
  // Draw indicator
  int16_t cx = epd->width() / 2;
  int16_t cy = epd->height() / 2;
  
  // Circle outline
  for (int angle = 0; angle < 360; angle += 5) {
    float rad = angle * 3.14159 / 180;
    int x = cx + cos(rad) * 30;
    int y = cy + sin(rad) * 30;
    epd->drawPixel(x, y, EPDColor::Black);
  }
  
  // Fill if ready, X if not
  if (hvReady) {
    // Checkmark
    for (int i = 0; i < 20; i++) {
      epd->drawPixel(cx - 10 + i, cy + i - 5, EPDColor::Black);
      if (i < 15) epd->drawPixel(cx - 10 + i, cy - i + 10, EPDColor::Black);
    }
    Serial.println("HV Ready: YES");
  } else {
    // X mark
    for (int i = 0; i < 30; i++) {
      epd->drawPixel(cx - 15 + i, cy - 15 + i, EPDColor::Black);
      epd->drawPixel(cx + 15 - i, cy - 15 + i, EPDColor::Black);
    }
    Serial.println("HV Ready: NO");
  }
  
  // VCOM Sense
  uint8_t vcom = epd->performVCOMSense();
  Serial.print("VCOM Sense: 0x");
  Serial.println(vcom, HEX);
  
  // Display VCOM value as bar
  int barWidth = vcom * 80 / 255;
  for (int i = 0; i < 80; i++) {
    epd->drawPixel(20 + i, cy + 50, EPDColor::Black);
    epd->drawPixel(20 + i, cy + 60, EPDColor::Black);
    if (i < barWidth) {
      for (int j = 51; j < 60; j++) {
        epd->drawPixel(20 + i, cy + j, EPDColor::Black);
      }
    }
  }
  
  epd->flushFull();
  delay(10000);
}
