#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1681)
#include "EPD_SSD1681.h"
#include <string.h>
#include <SPI.h>

static const uint8_t LUTDefault_full[] PROGMEM = {0x32, 0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x01, 0x00, 0x00, 0x00, 0x00};
static const uint8_t LUTDefault_part[] PROGMEM = {0x32, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t LUT_FULL_UPDATE[] PROGMEM = {0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00};
static const uint8_t LUT_PARTIAL_UPDATE[] PROGMEM = {0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// Fast partial LUT to reduce transitions and speed up partial refresh (Mode2)
static const uint8_t LUT_PARTIAL_FAST[] PROGMEM = {0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

EPD_SSD1681::EPD_SSD1681(const EPDConfig& cfg) : _cfg(cfg) {
  _phys_w = cfg.width;
  _phys_h = cfg.height;
}

EPD_SSD1681::~EPD_SSD1681() { end(); }

void EPD_SSD1681::allocateBuffer() {
  _fb_black_size = (size_t)_phys_w * (size_t)_phys_h / 8u;
  _fb_black = (uint8_t*)malloc(_fb_black_size);
  if (_fb_black) memset(_fb_black, 0xFF, _fb_black_size); // white = 1
  if (_cfg.variant == EPDVariant::BW_R || _cfg.variant == EPDVariant::BW_Y || _cfg.variant == EPDVariant::BW_RY) {
    _fb_accent_size = _fb_black_size; // same geometry (1bpp plane)
    _fb_accent = (uint8_t*)malloc(_fb_accent_size);
    if (_fb_accent) memset(_fb_accent, 0x00, _fb_accent_size); // default: no accent (0)
  }
}

void EPD_SSD1681::freeBuffer() {
  if (_fb_black) { free(_fb_black); _fb_black = nullptr; }
  if (_fb_accent) { free(_fb_accent); _fb_accent = nullptr; }
  _fb_black_size = 0; _fb_accent_size = 0;
}

uint8_t* EPD_SSD1681::framebuffer() { return _fb_black; }
size_t EPD_SSD1681::framebufferSize() const { return _fb_black_size; }
uint8_t* EPD_SSD1681::framebufferAccent() { return _fb_accent; }
size_t EPD_SSD1681::framebufferAccentSize() const { return _fb_accent_size; }

void EPD_SSD1681::setRotation(uint8_t r) { _rotation = (uint8_t)(r & 3); }

void EPD_SSD1681::mapXY(int16_t x, int16_t y, int16_t& xp, int16_t& yp) const {
  switch (_rotation & 3) {
    case 0: default: xp = x; yp = y; break;
    case 1: xp = y; yp = (int16_t)(_phys_h - 1) - x; break;       // 90°
    case 2: xp = (int16_t)(_phys_w - 1) - x; yp = (int16_t)(_phys_h - 1) - y; break; // 180°
    case 3: xp = (int16_t)(_phys_w - 1) - y; yp = x; break;       // 270°
  }
}

void EPD_SSD1681::mapRectToPhys(int16_t x, int16_t y, int16_t w, int16_t h,
                                int& xs, int& ys, int& xe, int& ye) const {
  // Map 4 corners and take axis-aligned bounding box in physical coords
  int16_t x0 = x;
  int16_t y0 = y;
  int16_t x1 = x + w - 1;
  int16_t y1 = y + h - 1;
  int16_t cx[4] = {x0, x1, x0, x1};
  int16_t cy[4] = {y0, y0, y1, y1};
  int16_t px, py;
  xs =  32767; ys =  32767;
  xe = -32768; ye = -32768;
  for (int i=0;i<4;++i) {
    mapXY(cx[i], cy[i], px, py);
    if (px < xs) xs = px;
    if (py < ys) ys = py;
    if (px > xe) xe = px;
    if (py > ye) ye = py;
  }
  // Clip to physical bounds
  if (xs < 0) xs = 0;
  if (ys < 0) ys = 0;
  if (xe >= (int)_phys_w) xe = _phys_w - 1;
  if (ye >= (int)_phys_h) ye = _phys_h - 1;
}

bool EPD_SSD1681::begin() {
  allocateBuffer();
  if (!_fb_black) return false;
  // Unified IO layer
  _io.begin();
  hwReset();
  delay(10); // Add delay from GxEPD2
  sendCommand(0x12); // SW reset
  delay(10);
  sendCommand(0x01); // Driver output control
  sendData((_phys_h - 1) & 0xFF);
  sendData(((_phys_h - 1) >> 8) & 0x01);
  sendData(0x00);
  sendCommand(0x11); // Data entry mode
  sendData(0x03);
  sendCommand(0x44); // Set RAM X address start/end
  sendData(0x00);
  sendData((_phys_w / 8) - 1);
  sendCommand(0x45); // Set RAM Y address start/end
  sendData(0x00);
  sendData(0x00);
  sendData((_phys_h - 1) & 0xFF);
  sendData(((_phys_h - 1) >> 8) & 0x01);
  setLUTForPanel(_profile); // Set LUT based on profile
  sendCommand(0x22); // Display update control 2
  sendData(0xC0); // From GxEPD2
  sendCommand(0x20);
  waitBusy();
  return true;
}

void EPD_SSD1681::end() {
  sleep();
  freeBuffer();
}

void EPD_SSD1681::sleep() {
  // Deep sleep
  sendCommand(0x10); // DEEP_SLEEP_MODE
  sendData(0x01);
  waitBusy();
}

bool EPD_SSD1681::isBusy() const {
  if (_cfg.busy < 0) return false;
  return digitalRead(_cfg.busy) == HIGH; // HIGH: busy (per vendor)
}

void EPD_SSD1681::drawPixel(int16_t x, int16_t y, EPDColor color) {
  if (!inBounds(x, y)) return;
  int16_t xp, yp;
  mapXY(x, y, xp, yp);
  
  switch (color) {
    case EPDColor::Black:
      // RAM polarity: 0 = black, 1 = white
      pixClr(_fb_black, xp, yp);
      if (_fb_accent) pixClr(_fb_accent, xp, yp);
      break;
    case EPDColor::White:
      pixSet(_fb_black, xp, yp);
      if (_fb_accent) pixClr(_fb_accent, xp, yp);
      break;
    case EPDColor::Red:
    case EPDColor::Yellow:
      if (_fb_accent) {
        pixClr(_fb_black, xp, yp);
        pixSet(_fb_accent, xp, yp);
      } else {
        // If no accent buffer, treat as black
        pixSet(_fb_black, xp, yp);
      }
      break;
  }
}

void EPD_SSD1681::fillScreen(EPDColor color) {
  switch (color) {
    case EPDColor::Black:
      memset(_fb_black, 0x00, _fb_black_size);
      if (_fb_accent) memset(_fb_accent, 0x00, _fb_accent_size);
      break;
    case EPDColor::White:
      memset(_fb_black, 0xFF, _fb_black_size);
      if (_fb_accent) memset(_fb_accent, 0x00, _fb_accent_size);
      break;
    case EPDColor::Red:
    case EPDColor::Yellow:
      if (_fb_accent) {
        memset(_fb_black, 0x00, _fb_black_size);
        memset(_fb_accent, 0xFF, _fb_accent_size);
      } else {
        // If no accent buffer, fill with black
        memset(_fb_black, 0xFF, _fb_black_size);
      }
      break;
  }
}

void EPD_SSD1681::setWindow(int x0, int y0, int x1, int y1) {
  // X in bytes (multiple of 8)
  sendCommand(0x44);
  sendData((uint8_t)((x0 >> 3) & 0xFF));
  sendData((uint8_t)((x1 >> 3) & 0xFF));
  sendCommand(0x45);
  sendData((uint8_t)(y0 & 0xFF));
  sendData((uint8_t)((y0 >> 8) & 0xFF));
  sendData((uint8_t)(y1 & 0xFF));
  sendData((uint8_t)((y1 >> 8) & 0xFF));
}

void EPD_SSD1681::setPointer(int x, int y) {
  sendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
  sendData((uint8_t)((x >> 3) & 0xFF));
  sendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
  sendData((uint8_t)(y & 0xFF));
  sendData((uint8_t)((y >> 8) & 0xFF));
  waitBusy();
}

void EPD_SSD1681::flushFull() {
  // Write full frame from _fb (assumed 200x200)
  setWindow(0, 0, _phys_w - 1, _phys_h - 1);
  for (uint16_t y = 0; y < _phys_h; ++y) {
    setPointer(0, y);
    sendCommand(0x24); // WRITE_RAM_BW
    const uint8_t* row = _fb_black + ((size_t)y * (size_t)_phys_w >> 3);
    sendDataBlock(row, (size_t)_phys_w >> 3);
  }
  // Accent plane (red/yellow)
  if (_fb_accent && (_cfg.variant == EPDVariant::BW_R || _cfg.variant == EPDVariant::BW_Y || _cfg.variant == EPDVariant::BW_RY)) {
    for (uint16_t y = 0; y < _phys_h; ++y) {
      setPointer(0, y);
      sendCommand(0x26); // WRITE_RAM_RED
      const uint8_t* row2 = _fb_accent + ((size_t)y * (size_t)_phys_w >> 3);
      sendDataBlock(row2, (size_t)_phys_w >> 3);
    }
  }
  // Trigger update with FULL profile (force full refresh)
  issueUpdate(RefreshProfile::Full);
}

void EPD_SSD1681::flushRect(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (w <= 0 || h <= 0) return;
  // Map logical rect to physical space (accounts for rotation)
  int xs, ys, xe, ye;
  mapRectToPhys(x, y, w, h, xs, ys, xe, ye);
  // Align X to byte boundaries on physical buffer
  int xs_al = xs & ~7;
  int xe_al = xe | 7;
  if (xe_al >= (int)_phys_w) xe_al = _phys_w - 1;

  setWindow(xs_al, ys, xe_al, ye);
  for (int yy = ys; yy <= ye; ++yy) {
    setPointer(xs_al, yy);
    sendCommand(0x24); // WRITE_RAM_BW
    const uint8_t* row = _fb_black + (((size_t)yy * (size_t)_phys_w + (size_t)xs_al) >> 3);
    size_t nbytes = (size_t)((xe_al - xs_al + 1) >> 3);
    sendDataBlock(row, nbytes);
  }
  if (_fb_accent) {
    for (int yy = ys; yy <= ye; ++yy) {
      setPointer(xs_al, yy);
      sendCommand(0x26); // WRITE_RAM_RED
      const uint8_t* row2 = _fb_accent + (((size_t)yy * (size_t)_phys_w + (size_t)xs_al) >> 3);
      size_t nbytes2 = (size_t)((xe_al - xs_al + 1) >> 3);
      sendDataBlock(row2, nbytes2);
    }
  }
  // Trigger update for partial region (uses partial profile flags)
  issueUpdate(RefreshProfile::Partial);
}

void EPD_SSD1681::updatePart(int16_t x, int16_t y, int16_t w, int16_t h) {
  if ((w <= 0) || (h <= 0)) return;
  int16_t x1 = x < 0 ? 0 : x;
  int16_t y1 = y < 0 ? 0 : y;
  int16_t w1 = x + w < (int16_t)_phys_w ? w : (int16_t)_phys_w - x;
  int16_t h1 = y + h < (int16_t)_phys_h ? h : (int16_t)_phys_h - y;
  x1 -= x1 % 8; // Byte boundary
  w1 = ((w1 + 7) / 8) * 8; // Byte boundary
  sendCommand(0x44); // Set RAM X address start/end
  sendData(x1 / 8);
  sendData((x1 + w1 - 1) / 8);
  sendCommand(0x45); // Set RAM Y address start/end
  sendData(y1 & 0xFF);
  sendData((y1 >> 8) & 0xFF);
  sendData((y1 + h1 - 1) & 0xFF);
  sendData(((y1 + h1 - 1) >> 8) & 0xFF);
  sendCommand(0x4E); // Set RAM X address counter
  sendData(x1 / 8);
  sendCommand(0x4F); // Set RAM Y address counter
  sendData(y1 & 0xFF);
  sendData((y1 >> 8) & 0xFF);
  sendCommand(0x24); // Write RAM for black plane
  for (int16_t i = 0; i < h1; i++) {
    const uint16_t yrow = (uint16_t)(y1 + i);
    const uint8_t* row = _fb_black + ((size_t)yrow * ((size_t)_phys_w / 8)) + (x1 / 8);
    sendDataBlock(row, w1 / 8);
  }
  // If accent buffer exists, handle it similarly, but for simplicity, focusing on black for now
  sendCommand(0x22); // Display update control
  sendData(0x04); // Partial update mode
  sendCommand(0x20); // Activate display update
  waitBusy();
}

void EPD_SSD1681::hwReset() { _io.hwReset(); }

void EPD_SSD1681::waitBusy() { _io.waitBusy(); }

void EPD_SSD1681::sendCommand(uint8_t c) { _io.sendCommand(c); }

void EPD_SSD1681::sendData(uint8_t d) { _io.sendData(d); }

void EPD_SSD1681::sendDataBlock(const uint8_t* p, size_t n) { if (!p || n==0) return; _io.sendDataBlock(p, n); }


void EPD_SSD1681::setRefreshProfile(RefreshProfile p) {
  _profile = p;
}

void EPD_SSD1681::setUpdateControlFlags(uint8_t full_flags, uint8_t partial_flags) {
  _upd_full_flags = full_flags;
  _upd_partial_flags = partial_flags;
}

void EPD_SSD1681::issueUpdate(RefreshProfile p) {
  uint8_t update_cmd = (p == RefreshProfile::Full) ? 0xF7 : 0xCF; // 0x04 = partial update
  sendCommand(0x22);
  sendData(update_cmd);
  sendCommand(0x20);
  waitBusy();
  delay(p == RefreshProfile::Full ? 100 : 30); // Use timings from GxEPD2
}

void EPD_SSD1681::setDisplayMode(DisplayMode m) {
  _disp_mode = m;
  // Default flags per mode (can be overridden via setUpdateControlFlags)
  if (_disp_mode == DisplayMode::Mode1) {
    _upd_full_flags = 0xC7; // Display with DISPLAY Mode 1
    _upd_partial_flags = 0xC7;
  } else {
    _upd_full_flags = 0xCF; // Display with DISPLAY Mode 2
    _upd_partial_flags = 0xCF;
  }
}

void EPD_SSD1681::reloadTempAndLUT(bool mode2) {
  // Load temperature and waveform LUT from OTP per datasheet
  // 0xB1 -> Mode 1, 0xB9 -> Mode 2
  sendCommand(0x22);
  sendData(mode2 ? 0xB9 : 0xB1);
  sendCommand(0x20);
  waitBusy();
}

void EPD_SSD1681::reloadLUTOnly(bool mode2) {
  // Load LUT with DISPLAY Mode 1/2 only (no temperature)
  // 0x91 -> Mode 1, 0x99 -> Mode 2
  sendCommand(0x22);
  sendData(mode2 ? 0x99 : 0x91);
  sendCommand(0x20);
  waitBusy();
}

void EPD_SSD1681::setDisplayUpdateControl1(uint8_t A, uint8_t B) {
  // Pass-through to DISPLAY_UPDATE_CONTROL_1 (0x21)
  sendCommand(0x21);
  sendData(A);
  sendData(B);
}

void EPD_SSD1681::setLUTForPanel(RefreshProfile profile) {
    const uint8_t* lut = nullptr;
    if (profile == RefreshProfile::Full) {
        lut = LUT_FULL_UPDATE;
    } else {
        // Partial: choose fast or default by display mode
        lut = (_disp_mode == DisplayMode::Mode2) ? LUT_PARTIAL_FAST : LUT_PARTIAL_UPDATE;
    }
    sendCommand(0x32); // WRITE_LUT
    for (size_t i = 0; i < 30; ++i) {
        sendData(pgm_read_byte(&lut[i]));
    }
}

#endif // EINK_SELECTIVE_BUILD / EINK_ENABLE_SSD1681
