/**
 * SSD1680 Demo 03: Partial Update Modes
 * Demonstrates partial refresh with different LUT modes
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

int counter = 0;

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
  
  // Initial full clear
  epd->fillScreen(EPDColor::White);
  epd->flushFull();
  
  // Set fast partial mode
  epd->setDisplayMode(DisplayMode::Mode2);
  epd->setRefreshProfile(RefreshProfile::Partial);
}

void drawCounter(int16_t x, int16_t y, int num) {
  // Draw large 3-digit counter
  char digits[4];
  sprintf(digits, "%03d", num);
  
  // Large 7-segment style digits
  for (int d = 0; d < 3; d++) {
    int dx = x + d * 30;
    int val = digits[d] - '0';
    
    // Segment patterns for 0-9
    const uint8_t segs[10] = {
      0x77, // 0: abcdef
      0x24, // 1: bc
      0x5D, // 2: abdeg
      0x6D, // 3: abcdg
      0x2E, // 4: bcfg
      0x6B, // 5: acdfg
      0x7B, // 6: acdefg
      0x25, // 7: abc
      0x7F, // 8: abcdefg
      0x6F  // 9: abcdfg
    };
    
    uint8_t s = segs[val];
    
    // Draw segments
    // Top (a)
    if (s & 0x01) for (int i = 0; i < 18; i++) epd->drawPixel(dx + 3 + i, y, EPDColor::Black);
    // Top right (b)
    if (s & 0x02) for (int i = 0; i < 12; i++) epd->drawPixel(dx + 21, y + 2 + i, EPDColor::Black);
    // Bottom right (c)
    if (s & 0x04) for (int i = 0; i < 12; i++) epd->drawPixel(dx + 21, y + 16 + i, EPDColor::Black);
    // Bottom (d)
    if (s & 0x08) for (int i = 0; i < 18; i++) epd->drawPixel(dx + 3 + i, y + 28, EPDColor::Black);
    // Bottom left (e)
    if (s & 0x10) for (int i = 0; i < 12; i++) epd->drawPixel(dx, y + 16 + i, EPDColor::Black);
    // Top left (f)
    if (s & 0x20) for (int i = 0; i < 12; i++) epd->drawPixel(dx, y + 2 + i, EPDColor::Black);
    // Middle (g)
    if (s & 0x40) for (int i = 0; i < 18; i++) epd->drawPixel(dx + 3 + i, y + 14, EPDColor::Black);
  }
}

void loop() {
  // Partial update region
  int16_t updateX = 10;
  int16_t updateY = 80;
  int16_t updateW = 100;
  int16_t updateH = 50;
  
  // Clear only the counter area
  for (int y = updateY; y < updateY + updateH; y++) {
    for (int x = updateX; x < updateX + updateW; x++) {
      epd->drawPixel(x, y, EPDColor::White);
    }
  }
  
  // Draw counter
  drawCounter(updateX + 5, updateY + 10, counter);
  
  // Partial update
  epd->flushRect(updateX, updateY, updateW, updateH);
  
  counter = (counter + 1) % 1000;
  
  // Every 50 updates, do a full refresh to clear artifacts
  if (counter % 50 == 0) {
    epd->fillScreen(EPDColor::White);
    epd->flushFull();
    delay(1000);
  }
  
  delay(500); // Update every 500ms
}
