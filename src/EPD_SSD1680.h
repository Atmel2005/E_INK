#pragma once
#include "EPD_Base.h"
#include <SPI.h>
#include "EPD_IO.h"

// Полноценный независимый драйвер SSD1680 по интерфейсу EPD_Base
class EPD_SSD1680 : public EPD_Base {
public:
  explicit EPD_SSD1680(const EPDConfig& cfg);
  ~EPD_SSD1680() override;

  // Core
  bool begin() override;
  void end() override;
  void sleep() override;
  bool isBusy() const override;

  // Drawing
  void setRotation(uint8_t r) override;
  void drawPixel(int16_t x, int16_t y, EPDColor color) override;
  void fillScreen(EPDColor color) override;

  // Updates
  void flushFull() override;
  void flushRect(int16_t x, int16_t y, int16_t w, int16_t h) override;

  // Framebuffers
  uint8_t* framebuffer() override { return _fb_black; }
  size_t framebufferSize() const override { return _fb_black_size; }
  uint8_t* framebufferAccent() override { return _fb_accent; }
  size_t framebufferAccentSize() const override { return _fb_accent_size; }

  void setRefreshProfile(RefreshProfile p) override { _profile = p; }
  void setDisplayMode(DisplayMode m) override { _disp_mode = m; }
  void setSterChar(int8_t px) override { _char_spacing = px; }
  int8_t getSterChar() const override { return _char_spacing; }

  uint16_t width() const override { return (_rotation & 1) ? _phys_h : _phys_w; }
  uint16_t height() const override { return (_rotation & 1) ? _phys_w : _phys_h; }

private:
  // Config/state
  EPDConfig _cfg;
  EPD_IO _io{_cfg};
  uint16_t _phys_w = 0, _phys_h = 0;
  uint8_t _rotation = 0;
  RefreshProfile _profile = RefreshProfile::Full;
  DisplayMode _disp_mode = DisplayMode::Mode1;
  int8_t _char_spacing = 0; // additional spacing between characters in pixels

  // Buffers
  uint8_t* _fb_black = nullptr;
  size_t _fb_black_size = 0;
  uint8_t* _fb_accent = nullptr;
  size_t _fb_accent_size = 0;

  // SPI via EPD_IO

  // Low-level
  void hwReset();
  void waitBusy();
  void sendCommand(uint8_t c);
  void sendData(uint8_t d);
  void sendDataBlock(const uint8_t* p, size_t n);
  void setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
  void setCursor(int16_t x, int16_t y);
  void issueUpdate(RefreshProfile p);

  void allocateBuffer();
  void freeBuffer();

  inline bool inBounds(int16_t x, int16_t y) const {
    return (x >= 0 && y >= 0 && (uint16_t)x < width() && (uint16_t)y < height());
  }
  inline void pixSet(uint8_t* fb, int16_t x, int16_t y) {
    const size_t bytes_per_row = (size_t)((_phys_w + 7) >> 3);
    size_t idx = (size_t)y * bytes_per_row + ((size_t)x >> 3);
    fb[idx] |= (0x80u >> (x & 7));
  }
  inline void pixClr(uint8_t* fb, int16_t x, int16_t y) {
    const size_t bytes_per_row = (size_t)((_phys_w + 7) >> 3);
    size_t idx = (size_t)y * bytes_per_row + ((size_t)x >> 3);
    fb[idx] &= ~(0x80u >> (x & 7));
  }

  void mapXY(int16_t x, int16_t y, int16_t& xp, int16_t& yp) const;
  void mapRectToPhys(int16_t x, int16_t y, int16_t w, int16_t h,
                     int16_t& xs, int16_t& ys, int16_t& xe, int16_t& ye) const;

  // SSD1680 specifics
  void _InitDisplay();
  void _Init_Full();
  void _Init_Part();
  void _Update_Full();
  void _Update_Part();
  void _PowerOn();
  void _PowerOff();
  void _setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
};
