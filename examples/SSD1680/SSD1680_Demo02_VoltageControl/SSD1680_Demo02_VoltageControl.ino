/**
 * SSD1680 Demo 02: Voltage Control
 * Demonstrates Gate, Source, and VCOM voltage adjustment
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

void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, EPDColor color) {
  // Border
  for (int16_t i = 0; i < w; i++) {
    epd->drawPixel(x + i, y, color);
    epd->drawPixel(x + i, y + h - 1, color);
  }
  for (int16_t i = 0; i < h; i++) {
    epd->drawPixel(x, y + i, color);
    epd->drawPixel(x + w - 1, y + i, color);
  }
  // Fill
  int16_t fill = (w - 4) * percent / 100;
  for (int16_t i = 2; i < fill + 2; i++) {
    for (int16_t j = 2; j < h - 2; j++) {
      epd->drawPixel(x + i, y + j, color);
    }
  }
}

void drawLabel(int16_t x, int16_t y, const char* label) {
  // Simple block letters for demo
  while (*label) {
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        if ((*label >> (i % 8)) & 1) {
          epd->drawPixel(x + j, y + i, EPDColor::Black);
        }
      }
    }
    x += 10;
    label++;
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
  Serial.println("Voltage Control Demo");
}

void loop() {
  // Test different voltage levels
  uint8_t vgh_levels[] = {0, 5, 10, 15, 20};  // Gate voltage
  uint8_t vcom_levels[] = {0, 10, 20, 30};     // VCOM levels
  
  for (int v = 0; v < 5; v++) {
    epd->fillScreen(EPDColor::White);
    
    // Set Gate Voltage (VGH)
    epd->setGateVoltage(vgh_levels[v]);
    
    // Draw VGH indicator
    Serial.print("VGH Level: ");
    Serial.println(vgh_levels[v]);
    
    // Progress bar for VGH (0-31 range)
    drawProgressBar(10, 20, 100, 15, vgh_levels[v] * 100 / 31, EPDColor::Black);
    
    // Set VCOM
    epd->setVCOM(vcom_levels[v % 4]);
    drawProgressBar(10, 50, 100, 15, vcom_levels[v % 4] * 100 / 255, EPDColor::Black);
    
    // Set Source Voltages
    epd->setSourceVoltage(0x41, 0xA8, 0x32);  // VSH1=15V, VSH2=5V, VSL=-15V
    drawProgressBar(10, 80, 100, 15, 50, EPDColor::Black);
    
    // Test pattern
    for (int i = 0; i < 100; i++) {
      epd->drawPixel(10 + i, 110 + (i % 20), EPDColor::Black);
    }
    
    // Border waveform test
    epd->setBorderWaveform(v % 4);
    
    epd->flushFull();
    delay(5000);
  }
  
  // Read back VCOM
  uint8_t vcom = epd->readVCOM();
  Serial.print("Read VCOM: 0x");
  Serial.println(vcom, HEX);
  
  delay(10000);
}
