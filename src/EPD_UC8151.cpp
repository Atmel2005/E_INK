#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_UC8151)
#include "EPD_UC8151.h"
#include <string.h>
#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/spi.h>
#endif

// UC8151 command set (minimal, aligned with SSD1608 for starting point)
#define UC_DRIVER_CONTROL   0x01
#define UC_SW_RESET         0x12
#define UC_WRITE_RAM_BW     0x24
#define UC_WRITE_RAM_ACC    0x26
#define UC_SET_RAMXPOS      0x44
#define UC_SET_RAMYPOS      0x45
#define UC_SET_RAMXCOUNT    0x4E
#define UC_SET_RAMYCOUNT    0x4F
#define UC_DISPLAY_UPDATE   0x21
#define UC_MASTER_ACT       0x20
#define UC_WRITE_LUT        0x32

static const uint8_t uc_lut_full[] = {
  0x02,0x02,0x01,0x11,0x12,0x12,0x22,0x22,
  0x66,0x69,0x69,0x59,0x58,0x99,0x99,0x88,
  0x00,0x00,0x00,0x00,0xF8,0xB4,0x13,0x51,
  0x35,0x51,0x51,0x19,0x01,0x00
};
static const uint8_t uc_lut_part[] = {
  0x10,0x18,0x18,0x08,0x18,0x18,0x08,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x13,0x14,0x44,0x12,
  0x00,0x00,0x00,0x00,0x00,0x00
};

EPD_UC8151::EPD_UC8151(const EPDConfig& cfg) : _cfg(cfg) {
  _phys_w = cfg.width;
  _phys_h = cfg.height;
  _rotation = cfg.rotation;
}

EPD_UC8151::~EPD_UC8151() { end(); }

bool EPD_UC8151::begin() {
  // Use unified IO layer
  _io.begin();

  hwReset();
  sendCommand(UC_SW_RESET);
  waitBusy();

  setResolution();
  setLUTByHost(true);
  setLUT(_profile == RefreshProfile::Full ? uc_lut_full : uc_lut_part);

  powerOn();

  allocateBuffer();
  fillScreen(EPDColor::White);
  flushFull();
  return true;
}

void EPD_UC8151::end() {
  sleep();
  _io.end();
}

void EPD_UC8151::sleep() {
  // Deep sleep по даташиту можно добавить позже
  delay(5);
}

bool EPD_UC8151::isBusy() const {
  return digitalRead(_cfg.busy) == HIGH;
}

void EPD_UC8151::setRotation(uint8_t r) {
  _rotation = r & 3;
  switch (_rotation) {
    case 0:
    case 2: _phys_w = _cfg.width; _phys_h = _cfg.height; break;
    case 1:
    case 3: _phys_w = _cfg.height; _phys_h = _cfg.width; break;
  }
}

void EPD_UC8151::drawPixel(int16_t x, int16_t y, EPDColor color) {
  if (!inBounds(x, y)) return;
  int16_t xp = x, yp = y;
  mapXY(x, y, xp, yp);
  switch (color) {
    case EPDColor::Black:   pixSet(_fb_black, xp, yp); break;
    case EPDColor::Red:
    case EPDColor::Yellow:  if (_cfg.variant != EPDVariant::BW) pixSet(_fb_accent, xp, yp); break;
    case EPDColor::White:
    default:                pixClr(_fb_black, xp, yp);
                            if (_cfg.variant != EPDVariant::BW) pixClr(_fb_accent, xp, yp);
                            break;
  }
}

void EPD_UC8151::fillScreen(EPDColor color) {
  uint8_t black_val = (color == EPDColor::Black || color == EPDColor::Red || color == EPDColor::Yellow) ? 0xFF : 0x00;
  memset(_fb_black, black_val, _fb_black_size);
  if (_cfg.variant != EPDVariant::BW) {
    uint8_t accent_val = ((color == EPDColor::Red && (_cfg.variant == EPDVariant::BW_R || _cfg.variant == EPDVariant::BW_RY)) ||
                          (color == EPDColor::Yellow && (_cfg.variant == EPDVariant::BW_Y || _cfg.variant == EPDVariant::BW_RY))) ? 0xFF : 0x00;
    memset(_fb_accent, accent_val, _fb_accent_size);
  }
}

void EPD_UC8151::flushFull() {
  if (!_fb_black) return;
  setWindow(0, 0, _phys_w - 1, _phys_h - 1);
  sendCommand(UC_WRITE_RAM_BW);
  sendDataBlock(_fb_black, _fb_black_size);
  if (_cfg.variant != EPDVariant::BW && _fb_accent) {
    sendCommand(UC_WRITE_RAM_ACC);
    sendDataBlock(_fb_accent, _fb_accent_size);
  }
  issueUpdate(RefreshProfile::Full);
}

void EPD_UC8151::flushRect(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!_fb_black || w <= 0 || h <= 0) return;
  int16_t xs, ys, xe, ye;
  mapRectToPhys(x, y, w, h, xs, ys, xe, ye);
  int16_t xs_al = xs & ~7;
  int16_t xe_al = xe | 7;
  if (xe_al >= _phys_w) xe_al = _phys_w - 1;
  setWindow(xs_al, ys, xe_al, ye);
  for (int16_t yy = ys; yy <= ye; ++yy) {
    sendCommand(UC_SET_RAMXCOUNT); sendData(xs_al >> 3);
    sendCommand(UC_SET_RAMYCOUNT); sendData(yy & 0xFF); sendData((yy >> 8) & 0x01);
    sendCommand(UC_WRITE_RAM_BW);
    const uint8_t* row = _fb_black + (((size_t)yy * (size_t)_phys_w + (size_t)xs_al) >> 3);
    size_t nbytes = (size_t)((xe_al - xs_al + 1) >> 3);
    sendDataBlock(row, nbytes);
  }
  if (_cfg.variant != EPDVariant::BW && _fb_accent) {
    for (int16_t yy = ys; yy <= ye; ++yy) {
      sendCommand(UC_SET_RAMXCOUNT); sendData(xs_al >> 3);
      sendCommand(UC_SET_RAMYCOUNT); sendData(yy & 0xFF); sendData((yy >> 8) & 0x01);
      sendCommand(UC_WRITE_RAM_ACC);
      const uint8_t* row2 = _fb_accent + (((size_t)yy * (size_t)_phys_w + (size_t)xs_al) >> 3);
      size_t nbytes2 = (size_t)((xe_al - xs_al + 1) >> 3);
      sendDataBlock(row2, nbytes2);
    }
  }
  issueUpdate(RefreshProfile::Partial);
}

// Low-level helpers
void EPD_UC8151::hwReset() { _io.hwReset(); waitBusy(); }
void EPD_UC8151::waitBusy() { _io.waitBusy(); }
void EPD_UC8151::sendCommand(uint8_t c) { _io.sendCommand(c); }
void EPD_UC8151::sendData(uint8_t d) { _io.sendData(d); }
void EPD_UC8151::sendDataBlock(const uint8_t* p, size_t n) { _io.sendDataBlock(p, n); }
void EPD_UC8151::setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  sendCommand(UC_SET_RAMXPOS); sendData(x0 >> 3); sendData(x1 >> 3);
  sendCommand(UC_SET_RAMYPOS);
  sendData(y0 & 0xFF); sendData((y0 >> 8) & 0x01);
  sendData(y1 & 0xFF); sendData((y1 >> 8) & 0x01);
  sendCommand(UC_SET_RAMXCOUNT); sendData(x0 >> 3);
  sendCommand(UC_SET_RAMYCOUNT);
  sendData(y0 & 0xFF); sendData((y0 >> 8) & 0x01);
}
void EPD_UC8151::setCursor(int16_t, int16_t) {}
void EPD_UC8151::issueUpdate(RefreshProfile p) {
  sendCommand(UC_DISPLAY_UPDATE);
  sendData(p == RefreshProfile::Full ? 0xC7 : 0x0F);
  sendCommand(UC_MASTER_ACT);
  waitBusy();
}
void EPD_UC8151::allocateBuffer() {
  freeBuffer();
  _fb_black_size = ((_phys_w + 7) / 8) * _phys_h;
  _fb_black = new uint8_t[_fb_black_size];
  if (_cfg.variant != EPDVariant::BW) {
    _fb_accent_size = _fb_black_size;
    _fb_accent = new uint8_t[_fb_accent_size];
  }
  memset(_fb_black, 0, _fb_black_size);
  if (_fb_accent) memset(_fb_accent, 0, _fb_accent_size);
}
void EPD_UC8151::freeBuffer() {
  if (_fb_black) { delete[] _fb_black; _fb_black = nullptr; _fb_black_size = 0; }
  if (_fb_accent) { delete[] _fb_accent; _fb_accent = nullptr; _fb_accent_size = 0; }
}
void EPD_UC8151::mapXY(int16_t x, int16_t y, int16_t& xp, int16_t& yp) const {
  switch (_rotation) {
    case 0: xp = x; yp = y; break;
    case 1: xp = _phys_w - y - 1; yp = x; break;
    case 2: xp = _phys_w - x - 1; yp = _phys_h - y - 1; break;
    case 3: xp = y; yp = _phys_h - x - 1; break;
  }
}
void EPD_UC8151::mapRectToPhys(int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t& xs, int16_t& ys, int16_t& xe, int16_t& ye) const {
  int16_t x1=x, y1=y, x2=x+w-1, y2=y+h-1, tmp;
  mapXY(x1, y1, xs, ys);
  mapXY(x2, y2, xe, ye);
  if (xs > xe) { tmp = xs; xs = xe; xe = tmp; }
  if (ys > ye) { tmp = ys; ys = ye; ye = tmp; }
}
void EPD_UC8151::setLUT(const uint8_t* lut) {
  sendCommand(UC_WRITE_LUT);
  for (int i=0;i<30;i++) sendData(lut[i]);
}
void EPD_UC8151::powerOn() {
  sendCommand(0x22); sendData(0xC0); sendCommand(0x20); waitBusy();
}
void EPD_UC8151::powerOff() {
  sendCommand(0x22); sendData(0x83); sendCommand(0x20); waitBusy();
}
void EPD_UC8151::setResolution() {
  sendCommand(0x61);
  sendData(_phys_w);
  sendData((_phys_h >> 8) & 0x01);
  sendData(_phys_h & 0xFF);
}
void EPD_UC8151::setLUTByHost(bool enabled) {
  sendCommand(0x22);
  sendData(enabled ? 0x40 : 0x00);
  sendCommand(0x20);
  waitBusy();
}
#endif // EINK_SELECTIVE_BUILD / EINK_ENABLE_UC8151
