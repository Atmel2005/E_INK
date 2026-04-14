#include <SPI.h>
#include <EPD_Base.h>

// Uncomment the controller you want to use
 //#define EPD_CONTROLLER EPDController::SSD1681
 #define EPD_CONTROLLER EPDController::SSD1680
// #define EPD_CONTROLLER EPDController::SSD1608
//#define EPD_CONTROLLER EPDController::UC8151
// #define EPD_CONTROLLER EPDController::IL3829  // Not implemented yet

// Pin mapping example (adjust for your board)
#if defined(ARDUINO_ARCH_RP2040)
  // RP2040 SW SPI preset similar to OLEDxxDMA style
  const int PIN_MOSI = 7;
  const int PIN_SCLK = 6;
  const int PIN_CS   = 10;
  const int PIN_DC   = 9;   // not used in 3-wire
  const int PIN_RST  = 8;
  const int PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  // ESP32 example pins (adjust to your board)
  const int PIN_MOSI = 15; // 23
  const int PIN_SCLK = 18;
  const int PIN_CS   = 5;
  const int PIN_DC   = 16;
  const int PIN_RST  = 17;
  const int PIN_BUSY = 4;
#else
  // UNO/AVR example SW SPI pins (adjust to wiring)
  const int PIN_MOSI = 11;
  const int PIN_SCLK = 13;
  const int PIN_CS   = 10;
  const int PIN_DC   = 9;
  const int PIN_RST  = 8;
  const int PIN_BUSY = 7;
#endif

// Подключаем GFX‑шрифты (латиница жирная 10pt и RUS жирная 10pt)
#ifndef GFXfont
typedef struct { uint16_t bitmapOffset; uint8_t  width, height; uint8_t xAdvance; int8_t xOffset, yOffset; } GFXglyph;
typedef struct { const uint8_t *bitmap; const GFXglyph *glyph; uint16_t first, last; uint8_t yAdvance; } GFXfont;
#endif
#include "../../Fonts/GFXFF/font_mod/lange/timesbd10pt7b.h"
#include "../../Fonts/GFXFF/font_mod/RUS/bold/tnr_rubd_10pt7b.h"
static const GFXfont* FONT_LAT_10 = &timesbd10pt7b;
static const GFXfont* FONT_RUS_10 = &tnr_rubd_10pt7b;

EPDConfig cfg;
EPD_Base* epd = nullptr;
static bool showLatin = true;
static unsigned long lastSwitch = 0;

// Рендеринг одного символа GFX
static void drawCharGFX(EPD_Base* epd, int16_t x, int16_t yBaseline, char c, EPDColor color, const GFXfont* fnt) {
  if (!epd || !fnt) return;
  if ((uint16_t)c < fnt->first || (uint16_t)c > fnt->last) return;
  const GFXglyph *g = &fnt->glyph[(uint16_t)c - fnt->first];
  uint8_t w = g->width, h = g->height;
  int8_t  xo = g->xOffset, yo = g->yOffset;
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

// Рисует текстовую строку указанным шрифтом
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

// Рисует N строк, укладываясь по ширине, для обычного ASCII‑шрифта
static int16_t drawGlyphsLines(EPD_Base* epd, const GFXfont* fnt, int16_t startY, uint8_t maxLines) {
  const int16_t W = epd->width();
  const int16_t marginX = 4;
  int16_t x = marginX;
  int16_t yBase = startY + fnt->yAdvance;
  uint8_t lines = 0;
  char line[96]; line[0] = 0; int li = 0;
  int8_t extra = epd ? epd->getSterChar() : 0;
  auto measure = [&](const char* s)->uint16_t{
    uint16_t w = 0; for (; *s; ++s) {
      char c = *s;
      if (c < fnt->first || c > fnt->last) { w += fnt->glyph[' ' - fnt->first].xAdvance + extra; continue; }
      w += fnt->glyph[c - fnt->first].xAdvance + extra;
    } return w;
  };
  for (uint16_t code = fnt->first; code <= fnt->last; ++code) {
    const GFXglyph* g = &fnt->glyph[code - fnt->first];
    if (g->xAdvance == 0 && g->width == 0) continue;
    char c = (char)code; char tmp[2] = { c, 0 };
    uint16_t curW = measure(line);
    uint16_t nextW = curW + measure(tmp);
    if (li > 0 && (x + (int)nextW + marginX > W)) {
      drawTextGFX(epd, x, yBase, line, EPDColor::Black, fnt);
      lines++; line[0] = 0; li = 0; yBase += fnt->yAdvance;
      if (lines >= maxLines) return yBase;
    }
    if (li < (int)sizeof(line) - 2) { line[li++] = c; line[li] = 0; }
  }
  if (li > 0 && lines < maxLines) {
    drawTextGFX(epd, x, yBase, line, EPDColor::Black, fnt);
    lines++; yBase += fnt->yAdvance;
  }
  return yBase;
}

// Отрисовка глифов по индексам (16‑битный диапазон кода)
static void drawGlyphByIndex(EPD_Base* epd, int16_t x, int16_t yBaseline, const GFXfont* fnt, uint16_t idx, EPDColor color) {
  if (!fnt) return;
  const GFXglyph *g = &fnt->glyph[idx];
  uint8_t w = g->width, h = g->height;
  int8_t  xo = g->xOffset, yo = g->yOffset;
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

static int16_t drawGlyphsLines16(EPD_Base* epd, const GFXfont* fnt, int16_t startY, uint8_t maxLines) {
  const int16_t W = epd->width();
  const int16_t marginX = 4;
  int16_t x = marginX;
  int16_t yBase = startY + fnt->yAdvance;
  uint8_t lines = 0;
  int8_t extra = epd ? epd->getSterChar() : 0;
  for (uint16_t code = fnt->first; code <= fnt->last; ++code) {
    uint16_t idx = code - fnt->first;
    const GFXglyph* g = &fnt->glyph[idx];
    if (g->xAdvance == 0 && g->width == 0) continue;
    if (x + g->xAdvance + extra + marginX > W) {
      lines++; x = marginX; yBase += fnt->yAdvance;
      if (lines >= maxLines) return yBase;
    }
    drawGlyphByIndex(epd, x, yBase, fnt, idx, EPDColor::Black);
    x += g->xAdvance + extra;
  }
  return yBase;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; } // Wait for serial port to connect
  
  Serial.println(F("\nEPD Intro Example"));
  
  // Configure the display
  cfg.width = 122;
  cfg.height = 250;
  cfg.variant = EPDVariant::BW_R; // Black/White display
  cfg.controller = EPD_CONTROLLER;
 
  
  // Configure SPI
  cfg.use_hw_spi = false; // Use SW SPI by default
  cfg.spi_hz = 4000000;   // 4 MHz
  
  // Configure pins
  cfg.cs = PIN_CS;
  cfg.dc = PIN_DC;
  cfg.rst = PIN_RST;
  cfg.busy = PIN_BUSY;
  
  if (!cfg.use_hw_spi) {
    cfg.mosi = PIN_MOSI;
    cfg.sclk = PIN_SCLK;
  }
  
  // 3-wire mode (if DC is not connected)
  // cfg.three_wire = true;
  
  // Create the appropriate EPD instance
  epd = createEPD(cfg);
  if (!epd) {
    Serial.println(F("Error: Failed to create EPD instance!"));
    while(1) { delay(1000); }
  }

  if (!epd->begin()) {
    Serial.println(F("EPD begin failed"));
    while (1) { delay(1000); }
  }

  // Режим частичных обновлений
  epd->setDisplayMode(DisplayMode::Mode2);
  epd->setSterChar(1);
  epd->setRotation(1); // landscape

  // Initial full clear
  epd->fillScreen(EPDColor::White);
  epd->flushFull();

  int16_t y = 4;
  y = drawGlyphsLines(epd, FONT_LAT_10, y, 4);
  epd->flushFull();
  lastSwitch = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSwitch >= 5000UL) {
    lastSwitch = now;
    epd->fillScreen(EPDColor::White);
    int16_t y = 4;
    if (showLatin) {
      // Показ кириллицы (RUS 10pt, диапазон 0x400..0x45F)
      y = drawGlyphsLines16(epd, FONT_RUS_10, y, 4);
    } else {
      // Показ латиницы (ASCII)
      y = drawGlyphsLines(epd, FONT_LAT_10, y, 4);
    }
    epd->flushFull();
    showLatin = !showLatin;
  }
}
