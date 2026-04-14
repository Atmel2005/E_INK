// Simple 2R demo for EPD displays (updated)
#include <Arduino.h>
#include <EPD_Base.h>
#if defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

#define EPD_CONTROLLER EPDController::SSD1680
// #define EPD_CONTROLLER EPDController::SSD1608
//#define EPD_CONTROLLER EPDController::UC8151

// Подключаем GFX‑шрифты из вашей библиотеки (kurze)
#ifndef GFXfont
typedef struct { uint16_t bitmapOffset; uint8_t  width, height; uint8_t xAdvance; int8_t xOffset, yOffset; } GFXglyph;
// Расширенная версия: first/last как uint16_t (поддержка 0x400..0x45F)
typedef struct { const uint8_t *bitmap; const GFXglyph *glyph; uint16_t first, last; uint8_t yAdvance; } GFXfont;
#endif

#include "../Fonts/GFXFF/font_mod/lange/timesbd10pt7b.h"
#include "../Fonts/GFXFF/font_mod/RUS/bold/tnr_rubd_10pt7b.h"
static const GFXfont* FONT_KURZE_10 = &timesbd10pt7b;
static const GFXfont* FONT_TNR_RU_10 = &tnr_rubd_10pt7b;

// Прототипы используемых функций
class EPD_Base; // forward decl
enum class EPDColor : uint8_t;
static void drawRedFrame(EPD_Base* epd);
static int16_t drawTextGFX(EPD_Base* epd, int16_t x, int16_t yBaseline, const char* s, EPDColor color, const GFXfont* fnt);

// Реализации
static void drawRedFrame(EPD_Base* epd) {
  int W = epd->width(), H = epd->height();
  for (int x=0; x<W; ++x) {
    epd->drawPixel(x, 0, EPDColor::Red);
    epd->drawPixel(x, 1, EPDColor::Red);
    epd->drawPixel(x, H-1, EPDColor::Red);
    epd->drawPixel(x, H-2, EPDColor::Red);
  }
  for (int y=0; y<H; ++y) {
    epd->drawPixel(0, y, EPDColor::Red);
    epd->drawPixel(1, y, EPDColor::Red);
    epd->drawPixel(W-1, y, EPDColor::Red);
    epd->drawPixel(W-2, y, EPDColor::Red);
  }
}

static void drawCharGFX(EPD_Base* epd, int16_t x, int16_t yBaseline, char c, EPDColor color, const GFXfont* fnt) {
  if (!fnt || c < fnt->first || c > fnt->last) return;
  const GFXglyph *glyph = &fnt->glyph[c - fnt->first];
  uint8_t w = glyph->width, h = glyph->height;
  int8_t xo = glyph->xOffset, yo = glyph->yOffset;
  const uint8_t* bm = fnt->bitmap + glyph->bitmapOffset;
  uint8_t bit = 0, bits = 0;
  for (uint8_t yy = 0; yy < h; yy++) {
    for (uint8_t xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) bits = pgm_read_byte(bm++);
      if (bits & 0x80) epd->drawPixel(x + xo + xx, yBaseline + yo + yy, color);
      bits <<= 1;
    }
  }
}

static int16_t drawTextGFX(EPD_Base* epd, int16_t x, int16_t yBaseline, const char* s, EPDColor color, const GFXfont* fnt) {
  if (!fnt) return x;
  int16_t cx = x;
  int8_t extra = epd ? epd->getSterChar() : 0;
  for (; *s; ++s) {
    char c = *s;
    if (c < fnt->first || c > fnt->last) {
      cx += fnt->glyph[' ' - fnt->first].xAdvance + extra;
      continue;
    }
    const GFXglyph *g = &fnt->glyph[c - fnt->first];
    drawCharGFX(epd, cx, yBaseline, c, color, fnt);
    cx += g->xAdvance + extra;
  }
  return cx;
}

// Измерение ширины строки по xAdvance с учётом дополнительного шага extra
static uint16_t measureTextWidth(const char* s, const GFXfont* fnt, int8_t extra) {
  if (!fnt) return 0;
  uint16_t w = 0;
  for (; *s; ++s) {
    char c = *s;
    if (c < fnt->first || c > fnt->last) {
      w += fnt->glyph[' ' - fnt->first].xAdvance + extra; continue;
    }
    w += fnt->glyph[c - fnt->first].xAdvance + extra;
  }
  return w;
}

// Рисует не более N строк глифов данного шрифта, укладываясь по ширине
static int16_t drawGlyphsLines(EPD_Base* epd, const GFXfont* fnt, int16_t startY, uint8_t maxLines) {
  const int16_t W = epd->width();
  const int16_t marginX = 4;
  int16_t x = marginX;
  int16_t yBase = startY + fnt->yAdvance;
  uint8_t lines = 0;
  char line[96]; line[0] = 0; int li = 0;
  int8_t extra = epd ? epd->getSterChar() : 0;
  for (uint16_t code = fnt->first; code <= fnt->last; ++code) {
    const GFXglyph* g = &fnt->glyph[code - fnt->first];
    if (g->xAdvance == 0 && g->width == 0) continue;
    char c = (char)code; char tmp[2] = { c, 0 };
    uint16_t curW = measureTextWidth(line, fnt, extra);
    uint16_t nextW = curW + measureTextWidth(tmp, fnt, extra);
    if (li > 0 && (x + (int)nextW + marginX > W)) {
      drawTextGFX(epd, x, yBase, line, EPDColor::Black, fnt);
      lines++; line[0] = 0; li = 0; yBase += fnt->yAdvance;
      if (lines >= maxLines) return yBase; // достигли лимита строк
    }
    if (li < (int)sizeof(line) - 2) { line[li++] = c; line[li] = 0; }
  }
  if (li > 0 && lines < maxLines) {
    drawTextGFX(epd, x, yBase, line, EPDColor::Black, fnt);
    lines++; yBase += fnt->yAdvance;
  }
  return yBase;
}

// Отрисовать конкретный глиф по индексу (без char), подходит для шрифтов с first > 255
static void drawGlyphByIndex(EPD_Base* epd, int16_t x, int16_t yBaseline, const GFXfont* fnt, uint16_t idx, EPDColor color) {
  const GFXglyph* g = &fnt->glyph[idx];
  uint8_t w = g->width, h = g->height; int8_t xo = g->xOffset, yo = g->yOffset;
  const uint8_t* bm = fnt->bitmap + g->bitmapOffset;
  uint8_t bit=0, bits=0;
  for (uint8_t yy=0; yy<h; yy++) {
    for (uint8_t xx=0; xx<w; xx++) {
      if (!(bit++ & 7)) bits = pgm_read_byte(bm++);
      if (bits & 0x80) epd->drawPixel(x+xo+xx, yBaseline+yo+yy, color);
      bits <<= 1;
    }
  }
}

// Рисует не более N строк для шрифта с 16-битным диапазоном кодов (например, 0x400..0x45F)
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

EPDConfig cfg;
EPD_Base* epd = nullptr;
static bool showLatin = true;
static unsigned long lastSwitch = 0;

void setup() {
  
  cfg.use_hw_spi = true;
#if defined(ARDUINO_ARCH_RP2040)
  // RP2040 SW SPI preset similar to OLEDxxDMA style
  cfg.mosi = 7;
  cfg.sclk = 6;
  cfg.cs   = 10;
  cfg.dc   = 9;   // not used in 3-wire
  cfg.rst  = 8;
  cfg.busy = 1; //11
#elif defined(ARDUINO_ARCH_ESP32)
  // ESP32 example pins (adjust to your board)
  cfg.mosi = 15; // 23  - 15
  cfg.sclk = 18;
  cfg.cs   = 5;
  cfg.dc   = 16;
  cfg.rst  = 17;
  cfg.busy = 4; //4
#else
  // UNO/AVR example SW SPI pins (adjust to wiring)
  cfg.mosi = 11;
  cfg.sclk = 13;
  cfg.cs   = 10;
  cfg.dc   = 9;
  cfg.rst  = 8;
  cfg.busy = 7;
#endif

  // 3-wire SPI switch (if your panel is set to 3-wire on DIP/solder switch)
  // cfg.three_wire = true; // uncomment when using 3-wire; DC pin will be ignored
  cfg.variant = EPDVariant::BW_R; // black + red panel
  cfg.spi_hz  = 4000000;          // HW SPI speed (ignored for SW SPI)
  cfg.controller = EPD_CONTROLLER; // Set the selected controller
  cfg.width = 122;
  cfg.height = 250;

  // Create the appropriate EPD instance
  epd = createEPD(cfg);
  if (!epd) {
    while (1) { 
      Serial.println(F("Error: Unsupported controller or init failed!"));
    }
  }

  if (!epd->begin()) {
    while (1) { 
      Serial.println(F("Display init failed!"));
    }
  }

  // Установить межсимвольный шаг: 1 пиксель
  epd->setSterChar(1);

  epd->setDisplayMode(DisplayMode::Mode2);
  epd->setRotation(1);
  epd->fillScreen(EPDColor::White);
  // Красная рамка по периметру
  drawRedFrame(epd);
  // Стартовая страница: 4 строки латиницы (kurze 10pt)
  int16_t y = 4; // верхнее поле
  y = drawGlyphsLines(epd, FONT_KURZE_10, y, 4);
  epd->setRefreshProfile(RefreshProfile::Full);
  epd->flushFull();
  lastSwitch = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSwitch >= 5000UL) {
    lastSwitch = now;
    // Переключаем страницу
    epd->fillScreen(EPDColor::White);
    drawRedFrame(epd);
    int16_t y = 4;
    if (showLatin) {
      // Показать кириллицу (RUS 10pt)
      y = drawGlyphsLines16(epd, FONT_TNR_RU_10, y, 4);
    } else {
      // Показать латиницу (kurze 10pt)
      y = drawGlyphsLines(epd, FONT_KURZE_10, y, 4);
    }
    epd->setRefreshProfile(RefreshProfile::Full);
    epd->flushFull();
    showLatin = !showLatin;
  }
}
