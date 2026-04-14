#include <SPI.h>
#include <Arduino.h>
#include <EPD_Base.h>
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif
#if defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

// Controller and variant: 2R (Black/White/Red)
#define EPD_CONTROLLER EPDController::SSD1680

// Fonts
#ifndef GFXfont
typedef struct { uint16_t bitmapOffset; uint8_t  width, height; uint8_t xAdvance; int8_t xOffset, yOffset; } GFXglyph;
typedef struct { const uint8_t *bitmap; const GFXglyph *glyph; uint16_t first, last; uint8_t yAdvance; } GFXfont;
#endif
#include "../../Fonts/GFXFF/font_mod/lange/timesbd29pt7b.h"   // ASCII bold 29pt for HH:MM:SS

// Красная рамка по периметру (как в рабочем демо)
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

// Simple GFX text rendering (ASCII range)
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

static uint16_t measureTextWidth(const char* s, const GFXfont* fnt, int8_t extra) {
  if (!fnt) return 0; uint16_t w = 0;
  for (; *s; ++s) {
    char c = *s;
    if (c < fnt->first || c > fnt->last) { w += fnt->glyph[' ' - fnt->first].xAdvance + extra; continue; }
    w += fnt->glyph[c - fnt->first].xAdvance + extra;
  }
  return w;
}

static int16_t drawTextGFX(EPD_Base* epd, int16_t x, int16_t yBaseline, const char* s, EPDColor color, const GFXfont* fnt) {
  if (!fnt) return x; int16_t cx = x; int8_t extra = epd ? epd->getSterChar() : 0;
  for (; *s; ++s) {
    char c = *s;
    if (c < fnt->first || c > fnt->last) { cx += fnt->glyph[' ' - fnt->first].xAdvance + extra; continue; }
    const GFXglyph *g = &fnt->glyph[c - fnt->first];
    drawCharGFX(epd, cx, yBaseline, c, color, fnt);
    cx += g->xAdvance + extra;
  }
  return cx;
}

// Объявление функции; определение ниже, после глобальных переменных
static void drawAllTime();

// Pins (adjust to your board)
#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7;
  const int PIN_SCLK = 6;
  const int PIN_CS   = 10;
  const int PIN_DC   = 9;   // not used in 3-wire
  const int PIN_RST  = 8;
  const int PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 23; // 23
  const int PIN_SCLK = 18;
  const int PIN_CS   = 5;
  const int PIN_DC   = 17;
  const int PIN_RST  = 16;
  const int PIN_BUSY = 4;
#else
  const int PIN_MOSI = 11;
  const int PIN_SCLK = 13;
  const int PIN_CS   = 10;
  const int PIN_DC   = 9;
  const int PIN_RST  = 8;
  const int PIN_BUSY = 7;
#endif

EPDConfig cfg;
EPD_Base* epd = nullptr;

// Time state (simple millis-driven clock)
static uint8_t hh = 23, mm = 59, ss = 59;
static unsigned long lastTick = 0;

// Fixed boxes for segments (no horizontal shifting)
static int16_t yBase = 0;            // общий базовый уровень
static int16_t boxW = 0;             // ширина двух цифр (макс, например "88")
static int16_t colonW = 0;           // ширина ':'
static int16_t boxH = 0;             // высота строки (yAdvance)
static int16_t xHH = 0, xC1 = 0, xMM = 0, xC2 = 0, xSS = 0; // позиции сегментов

// Center layout boxes
static void layoutBoxes() {
  const GFXfont* font = &timesbd29pt7b;
  int8_t extra = epd->getSterChar();
  boxW   = (int16_t)measureTextWidth("88", font, extra); // худший случай
  colonW = (int16_t)measureTextWidth(":",  font, extra);
  boxH   = font->yAdvance;
  uint16_t W = epd->width();
  uint16_t H = epd->height();
  int16_t totalW = boxW + colonW + boxW + colonW + boxW;
  int16_t x0 = (int16_t)((int)W - (int)totalW) / 2;
  int16_t y0 = (int16_t)((int)H - (int)boxH) / 2;
  yBase = y0 + boxH;
  xHH = x0;
  xC1 = xHH + boxW;
  xMM = xC1 + colonW;
  xC2 = xMM + boxW;
  xSS = xC2 + colonW;
}

// Полная отрисовка текущего времени HH:MM:SS
static void drawAllTime() {
  const GFXfont* font = &timesbd29pt7b;
  char bHH[4], bMM[4], bSS[4];
  snprintf(bHH, sizeof(bHH), "%02u", hh);
  snprintf(bMM, sizeof(bMM), "%02u", mm);
  snprintf(bSS, sizeof(bSS), "%02u", ss);
  drawTextGFX(epd, xHH, yBase, bHH, EPDColor::Black, font);
  drawTextGFX(epd, xC1, yBase, ":",  EPDColor::Black, font);
  drawTextGFX(epd, xMM, yBase, bMM, EPDColor::Black, font);
  drawTextGFX(epd, xC2, yBase, ":",  EPDColor::Black, font);
  drawTextGFX(epd, xSS, yBase, bSS, EPDColor::Black, font);
}

// Очистка прямоугольника [x, x+boxW) × [yBase-boxH, yBase) в белый (без flush)
static void clearBox(int16_t xStart) {
  int16_t xs = xStart - 2; if (xs < 0) xs = 0;
  int16_t ys = yBase - boxH;
  if (ys < 0) ys = 0;
  int16_t w  = boxW + 4; if (xs + w > (int16_t)epd->width()) w = epd->width() - xs;
  int16_t h  = boxH + 2; if (ys + h > (int16_t)epd->height()) h = epd->height() - ys;
  for (int16_t y = ys; y < ys + h; ++y)
    for (int16_t x = xs; x < xs + w; ++x)
      epd->drawPixel(x, y, EPDColor::White);
}

// Перерисовать сегмент (2 цифры) цветом и выполнить частичный flush
static void redrawSegment(int16_t xStart, const char* txt, EPDColor color) {
  const GFXfont* font = &timesbd29pt7b;
  // Очистка без мгновенного flush, чтобы избежать двойного мерцания
  clearBox(xStart);
  drawTextGFX(epd, xStart, yBase, txt, color, font);
  // Flush только этот бокс
  int16_t xs = xStart - 2; if (xs < 0) xs = 0;
  int16_t ys = yBase - boxH; if (ys < 0) ys = 0;
  int16_t w  = boxW + 4; if (xs + w > (int16_t)epd->width()) w = epd->width() - xs;
  int16_t h  = boxH + 2; if (ys + h > (int16_t)epd->height()) h = epd->height() - ys;
  epd->flushRect(xs, ys, w, h);
}

// Перерисовать двоеточие и локально обновить
static void redrawColon(int16_t xStart) {
  const GFXfont* font = &timesbd29pt7b;
  // Очистка области двоеточия
  int16_t xs = xStart - 2; if (xs < 0) xs = 0;
  int16_t ys = yBase - boxH; if (ys < 0) ys = 0;
  int16_t w  = colonW + 4; if (xs + w > (int16_t)epd->width()) w = epd->width() - xs;
  int16_t h  = boxH + 2; if (ys + h > (int16_t)epd->height()) h = epd->height() - ys;
  for (int16_t y = ys; y < ys + h; ++y)
    for (int16_t x = xs; x < xs + w; ++x)
      epd->drawPixel(x, y, EPDColor::White);
  drawTextGFX(epd, xStart, yBase, ":", EPDColor::Black, font);
  epd->flushRect(xs, ys, w, h);
}

static void tickClock() {
  unsigned long now = millis();
  if (now - lastTick >= 1000UL) {
    lastTick = now;
    ss++;
    if (ss >= 60) { ss = 0; mm++; if (mm >= 60) { mm = 0; hh = (hh + 1) % 24; } }
  }
}

void setup() {
  // ИНИЦИАЛИЗАЦИЯ — как в рабочем EPD_SSD1681_2R_Demo
  // UNO/AVR example SW SPI pins (adjust to wiring)
  cfg.use_hw_spi = true;
  cfg.mosi = PIN_MOSI;
  cfg.sclk = PIN_SCLK;
  cfg.cs   = PIN_CS;
  cfg.dc   = PIN_DC;
  cfg.rst  = PIN_RST;
  cfg.busy = PIN_BUSY;

  // 3-wire SPI switch (если панель выставлена в 3-wire)
  // cfg.three_wire = true;
  cfg.variant = EPDVariant::BW;      // временно чёрно-белый режим для стабильных partial
  cfg.spi_hz  = 4000000;             // 4 MHz как в рабочем демо
  cfg.controller = EPD_CONTROLLER;   // SSD1680
  cfg.width = 122;
  cfg.height = 250;

  epd = createEPD(cfg);
  if (!epd) {
    while (1) {  }
  }
  if (!epd->begin()) {
    while (1) {  }
  }
  epd->setSterChar(1);
  epd->setDisplayMode(DisplayMode::Mode1);
  epd->setRotation(1); // landscape, как в рабочем демо

  // Рассчитать компоновку боксов и первично вывести все сегменты
  layoutBoxes();
  epd->fillScreen(EPDColor::White);
  // Временно отключено: не рисуем красную рамку, чтобы исключить влияние цветного слоя
  // drawRedFrame(epd);
  // Рисуем время без красного слоя для диагностики
  {
    const GFXfont* font = &timesbd29pt7b;
    char bHH[4], bMM[4], bSS[4];
    snprintf(bHH, sizeof(bHH), "%02u", hh);
    snprintf(bMM, sizeof(bMM), "%02u", mm);
    snprintf(bSS, sizeof(bSS), "%02u", ss);
    drawTextGFX(epd, xHH, yBase, bHH, EPDColor::Black, font);
    drawTextGFX(epd, xC1, yBase, ":",  EPDColor::Black, font);
    drawTextGFX(epd, xMM, yBase, bMM, EPDColor::Black, font);
    drawTextGFX(epd, xC2, yBase, ":",  EPDColor::Black, font);
    drawTextGFX(epd, xSS, yBase, bSS, EPDColor::Red,   font);
  }
  // Отправляем стартовый кадр полностью (один раз)
  epd->setRefreshProfile(RefreshProfile::Full);
  epd->flushFull();
  lastTick = millis();
}

void loop() {
  tickClock();
  static uint8_t prev_ss = 255;
  if (ss != prev_ss) {
    prev_ss = ss;
    // Полная перерисовка: белый фон + время (всё чёрным)
    epd->fillScreen(EPDColor::White);
    drawAllTime();
    epd->setRefreshProfile(RefreshProfile::Full);
    epd->flushFull();
  }
}
