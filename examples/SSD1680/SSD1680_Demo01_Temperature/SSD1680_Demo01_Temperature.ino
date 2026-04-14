/**
 * SSD1680 Demo 01: Temperature Display
 * Shows internal temperature sensor and auto-LUT feature
 */

#include <SPI.h>
#include <EPD_Base.h>

#define EPD_CONTROLLER EPDController::SSD1680

// Pin configuration
#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7, PIN_SCLK = 6, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 6, PIN_SCLK = 4, PIN_CS = 7, PIN_DC = 5, PIN_RST = 2, PIN_BUSY = 3;
#else
  const int PIN_MOSI = 11, PIN_SCLK = 13, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 7;
#endif

EPDConfig cfg;
EPD_Base* epd = nullptr;

// Simple 5x7 font for numbers
const uint8_t font5x7[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, // space
  0x00, 0x00, 0x5F, 0x00, 0x00, // !
  // ... simplified for demo
  0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
  0x00, 0x42, 0x7F, 0x40, 0x00, // 1
  0x42, 0x61, 0x51, 0x49, 0x46, // 2
  0x21, 0x41, 0x45, 0x4B, 0x31, // 3
  0x18, 0x14, 0x12, 0x7F, 0x10, // 4
  0x27, 0x45, 0x45, 0x45, 0x39, // 5
  0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
  0x01, 0x71, 0x09, 0x05, 0x03, // 7
  0x36, 0x49, 0x49, 0x49, 0x36, // 8
  0x06, 0x49, 0x49, 0x29, 0x1E, // 9
};

void drawChar(int16_t x, int16_t y, char c, EPDColor color) {
  if (c < '0' || c > '9') return;
  uint16_t idx = (c - '0' + 3) * 5; // offset to numbers
  for (uint8_t col = 0; col < 5; col++) {
    uint8_t line = pgm_read_byte(&font5x7[idx + col]);
    for (uint8_t row = 0; row < 7; row++) {
      if (line & (1 << row)) {
        epd->drawPixel(x + col, y + row, color);
      }
    }
  }
}

void drawText(int16_t x, int16_t y, const char* text, EPDColor color) {
  while (*text) {
    drawChar(x, y, *text, color);
    x += 6;
    text++;
  }
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, EPDColor color) {
  int16_t dx = abs(x1 - x0), dy = abs(y1 - y0);
  int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int16_t err = dx - dy;
  while (true) {
    epd->drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}

void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, EPDColor color) {
  for (int16_t i = x; i < x + w; i++) {
    epd->drawPixel(i, y, color);
    epd->drawPixel(i, y + h - 1, color);
  }
  for (int16_t i = y; i < y + h; i++) {
    epd->drawPixel(x, i, color);
    epd->drawPixel(x + w - 1, i, color);
  }
}

// Temperature callback
void onTemperatureRead(int16_t temp) {
  Serial.print("Temperature: ");
  Serial.print(temp / 100.0);
  Serial.println(" C");
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
  
  // Enable auto-temperature LUT
  epd->setTempCallback(onTemperatureRead);
  epd->enableAutoTempLUT(true);
  
  epd->setRotation(1);
}

void loop() {
  epd->fillScreen(EPDColor::White);
  
  // Draw thermometer icon
  int16_t cx = 20, cy = epd->height() / 2;
  drawRect(cx - 8, cy - 40, 16, 50, EPDColor::Black);
  for (int i = 0; i < 5; i++) {
    epd->drawPixel(cx - 6 + i, cy + 10 + i, EPDColor::Black);
    epd->drawPixel(cx + 6 - i, cy + 10 + i, EPDColor::Black);
  }
  
  // Read temperature (triggers callback)
  int16_t temp = epd->readTemperature();
  int8_t range = epd->getTemperatureRange(temp);
  
  // Display temperature
  char buf[16];
  int t = temp / 100;
  int d = abs(temp % 100);
  sprintf(buf, "%d.%02d C", t, d);
  drawText(45, cy - 10, buf, EPDColor::Black);
  
  // Display range
  const char* ranges[] = {"<0C", "0-10C", "10-25C", "25-40C", "40-50C", ">50C"};
  drawText(45, cy + 10, ranges[range], EPDColor::Black);
  
  // Temperature bar
  int16_t barY = cy + 30;
  drawRect(45, barY, 60, 10, EPDColor::Black);
  int16_t fill = map(constrain(temp, -1000, 6000), -1000, 6000, 0, 58);
  for (int i = 1; i < fill; i++) {
    for (int j = 1; j < 9; j++) {
      epd->drawPixel(46 + i, barY + j, EPDColor::Black);
    }
  }
  
  epd->flushFull();
  delay(30000); // Update every 30 seconds
}
