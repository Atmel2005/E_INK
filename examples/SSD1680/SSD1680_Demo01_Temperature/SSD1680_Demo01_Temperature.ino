/**
 * SSD1680 Demo 01: Temperature Display
 * Shows internal temperature sensor and auto-LUT feature
 * Resolution: 122x250
 */

#include <SPI.h>
#include <EPD_Base.h>

#define EPD_CONTROLLER EPDController::SSD1680

#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7, PIN_SCLK = 6, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 8, PIN_SCLK = 9, PIN_CS = 10, PIN_DC = 11, PIN_RST = 12, PIN_BUSY = 13;
#else
  const int PIN_MOSI = 11, PIN_SCLK = 13, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 7;
#endif

// GFX font support
#ifndef GFXfont
typedef struct { uint16_t bitmapOffset; uint8_t width, height; uint8_t xAdvance; int8_t xOffset, yOffset; } GFXglyph;
typedef struct { const uint8_t *bitmap; const GFXglyph *glyph; uint16_t first, last; uint8_t yAdvance; } GFXfont;
#endif
#include "../../../Fonts/GFXFF/FreeSerifBold18pt7b.h"
static const GFXfont* FONT_BIG = &FreeSerifBold18pt7b;
#include "../../../Fonts/GFXFF/FreeSerifBold12pt7b.h"
static const GFXfont* FONT_SMALL = &FreeSerifBold12pt7b;

EPDConfig cfg;
EPD_Base* epd = nullptr;

// Render one GFX character
static void drawCharGFX(EPD_Base* epd, int16_t x, int16_t yBaseline, char c, EPDColor color, const GFXfont* fnt) {
  if (!epd || !fnt) return;
  if ((uint16_t)c < fnt->first || (uint16_t)c > fnt->last) return;
  const GFXglyph *g = &fnt->glyph[(uint16_t)c - fnt->first];
  uint8_t w = g->width, h = g->height;
  int8_t xo = g->xOffset, yo = g->yOffset;
  const uint8_t* bm = fnt->bitmap + g->bitmapOffset;
  uint8_t bits = 0, bit = 0;
  for (uint8_t yy = 0; yy < h; ++yy) {
    for (uint8_t xx = 0; xx < w; ++xx) {
      if (!(bit++ & 7)) bits = pgm_read_byte(bm++);
      if (bits & 0x80) epd->drawPixel(x + xo + xx, yBaseline + yo + yy, color);
      bits <<= 1;
    }
  }
}

// Draw text string with GFX font
static int16_t drawTextGFX(EPD_Base* epd, int16_t x, int16_t yBaseline, const char* s, EPDColor color, const GFXfont* fnt) {
  if (!fnt) return x;
  int16_t cx = x;
  int8_t extra = epd ? epd->getSterChar() : 0;
  for (; *s; ++s) {
    char c = *s;
    if (c < fnt->first || c > fnt->last) {
      cx += fnt->glyph[' ' - fnt->first].xAdvance + extra; continue;
    }
    const GFXglyph *g = &fnt->glyph[c - fnt->first];
    drawCharGFX(epd, cx, yBaseline, c, color, fnt);
    cx += g->xAdvance + extra;
  }
  return cx;
}

void drawThermometer(int16_t x, int16_t y, int16_t temp) {
  // Thermometer outline
  for (int i = -8; i <= 8; i++) {
    for (int j = 0; j < 60; j++) {
      if (abs(i) == 8 || j == 0 || (j >= 55 && i*i + (j-55)*(j-55) <= 64)) {
        epd->drawPixel(x + i, y + j, EPDColor::Black);
      }
    }
  }
  
  // Fill based on temperature
  int fill = map(constrain(temp, -1000, 6000), -1000, 6000, 5, 50);
  for (int j = 55; j > 55 - fill; j--) {
    for (int i = -6; i <= 6; i++) {
      epd->drawPixel(x + i, y + j, EPDColor::Black);
    }
  }
}

void onTempChange(int16_t temp) {
  // Temperature callback - called when temperature changes
}

void setup() {
  Serial.begin(115200);
  
  cfg.width = 122;
  cfg.height = 250;
  cfg.variant = EPDVariant::BW_R;
  cfg.controller = EPD_CONTROLLER;
  cfg.use_hw_spi = false;
  cfg.cs = PIN_CS; cfg.dc = PIN_DC; cfg.rst = PIN_RST;
  cfg.busy = PIN_BUSY; cfg.mosi = PIN_MOSI; cfg.sclk = PIN_SCLK;
  
  epd = createEPD(cfg);
  if (!epd || !epd->begin()) {
    Serial.println("EPD init failed!");
    while(2);
  }
  
  epd->setTempCallback(onTempChange);
  epd->enableAutoTempLUT(true);
  epd->setRotation(2);
}

void loop() {
  // Try to read temperature, but also set manually for testing
  int16_t temp = epd->readTemperature();
  
  // If temp is 0, use a simulated value for demo
  static int16_t simTemp = 2000;  // Start at 20°C
  if (temp == 0) {
    temp = simTemp;
    simTemp += 100;  // Increment by 1°C each cycle
    if (simTemp > 4000) simTemp = 2000;  // Reset at 40°C
  }
  
  // Set temperature for LUT selection
  epd->setTemperature(temp);
  
  epd->fillScreen(EPDColor::White);
  
  // Draw thermometer
  drawThermometer(10, 20, temp);
  
  // Draw temperature with big font
  char buf[16];
  int t = temp / 100;
  int d = abs(temp % 100);
  if (temp < 0) {
    sprintf(buf, "-%d.%02d C", -t, d);
  } else {
    sprintf(buf, "%d.%02d C", t, d);
  }
  drawTextGFX(epd, 50, 55, buf, EPDColor::Black, FONT_BIG);
  
  epd->flushFull();
  delay(30000);
}
