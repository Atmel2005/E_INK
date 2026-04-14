#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1680)
#include "EPD_SSD1680.h"
#include <Arduino.h>
#include <string.h>
#include <SPI.h>

// Waveform LUTs adapted from GxEPD2 for SSD168x family
static const uint8_t LUTDefault_full[] PROGMEM = {
  0x32, 0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
  0x01, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t LUTDefault_part[] PROGMEM = {
  0x32, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00
};

// Faster partial update LUT (reduced transitions), inspired by GxEPD2 fast waveforms
static const uint8_t LUTFast_part[] PROGMEM = {
  0x10, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00
};

EPD_SSD1680::EPD_SSD1680(const EPDConfig& cfg) : _cfg(cfg) {
  _phys_w = cfg.width;
  _phys_h = cfg.height;
  _rotation = cfg.rotation;
}

EPD_SSD1680::~EPD_SSD1680() { end(); }

bool EPD_SSD1680::begin() {
  _io.begin();
  hwReset();
  _Init_Full();
  _PowerOn();
  allocateBuffer();
  // Оставляем экран как есть; далее пользовательский код рисует и вызывает flush*
  return true;
}

void EPD_SSD1680::end() {
  sleep();
  _io.end();
}

void EPD_SSD1680::sleep() {
  // Deep sleep
  sendCommand(0x10);
  sendData(0x01);
  delay(100);
}

bool EPD_SSD1680::isBusy() const {
  return digitalRead(_cfg.busy) == HIGH;
}

void EPD_SSD1680::setRotation(uint8_t r) {
  // Remap: 0->1, 1->0, 2->3, 3->2 (same as SSD1681)
  static const uint8_t map[4] = {1, 0, 3, 2};
  _rotation = map[r & 3];
  // Не меняем _phys_w/_phys_h: геометрия контроллера фиксирована (WIDTH=122 видимая, CTRL_W=128, HEIGHT=250).
  // Поворот учитывается только в mapXY()/mapRectToPhys().
}

void EPD_SSD1680::drawPixel(int16_t x, int16_t y, EPDColor color) {
  if (!inBounds(x, y)) return;
  int16_t xp = x, yp = y;
  mapXY(x, y, xp, yp);
  switch (color) {
    case EPDColor::Black:
      // 1 = white, 0 = black
      pixClr(_fb_black, xp, yp);
      break;
    case EPDColor::Red:
    case EPDColor::Yellow:
      // Цвет задаётся через инверсию при отправке (~data),
      // поэтому в буфере 0 = цвет, 1 = нет цвета
      if (_cfg.variant != EPDVariant::BW) pixClr(_fb_accent, xp, yp);
      // и чёрный не ставим, оставляем белым
      pixSet(_fb_black, xp, yp);
      break;
    case EPDColor::White:
    default:
      pixSet(_fb_black, xp, yp);
      if (_cfg.variant != EPDVariant::BW) pixSet(_fb_accent, xp, yp);
      break;
  }
}

void EPD_SSD1680::fillScreen(EPDColor color) {
  if (!_fb_black) return;
  // Black plane: 1 = white, 0 = black
  uint8_t black_val = (color == EPDColor::Black) ? 0x00 : 0xFF;
  memset(_fb_black, black_val, _fb_black_size);
  if (_cfg.variant != EPDVariant::BW) {
    // For color plane we later send inverted bytes (~data).
    // To have NO color on screen we need to send 0x00 -> data must be 0xFF (so ~0xFF=0x00).
    // To have color on screen we need to send 0xFF -> data must be 0x00 (so ~0x00=0xFF).
    bool want_color = (color == EPDColor::Red && (_cfg.variant == EPDVariant::BW_R || _cfg.variant == EPDVariant::BW_RY)) ||
                      (color == EPDColor::Yellow && (_cfg.variant == EPDVariant::BW_Y || _cfg.variant == EPDVariant::BW_RY));
    uint8_t accent_val = want_color ? 0x00 : 0xFF;
    if (_fb_accent) memset(_fb_accent, accent_val, _fb_accent_size);
  }
}

void EPD_SSD1680::flushFull() {
  if (!_fb_black) return;
  _Init_Part();
  // Полное окно 128x_height, как в GxEPD2
  _setPartialRamArea(0, 0, 128, _phys_h);
  // Ч/б плоскость целиком, единым потоком
  sendCommand(0x24);
  const size_t bytes_per_row_vis = (size_t)((_phys_w + 7) >> 3);
  const size_t bytes_per_row = (size_t)128 >> 3; // 16
  for (uint16_t yy = 0; yy < _phys_h; ++yy) {
    const uint8_t* row = _fb_black + ((size_t)yy * bytes_per_row_vis);
    sendDataBlock(row, bytes_per_row_vis);
    for (size_t i = bytes_per_row_vis; i < bytes_per_row; ++i) sendData(0xFF);
  }
  // Цветная плоскость целиком (инвертированная), как в GxEPD2
  if (_cfg.variant != EPDVariant::BW && _fb_accent) {
    sendCommand(0x26);
    for (uint16_t yy = 0; yy < _phys_h; ++yy) {
      const uint8_t* row2 = _fb_accent + ((size_t)yy * bytes_per_row_vis);
      for (size_t i=0;i<bytes_per_row_vis;++i) sendData(~row2[i]);
      for (size_t i = bytes_per_row_vis; i < bytes_per_row; ++i) sendData(0x00);
    }
  }
  issueUpdate(RefreshProfile::Full);
}

void EPD_SSD1680::syncToRAM() {
  const size_t bytes_per_row = (size_t)((_phys_w + 7) >> 3);
  _setPartialRamArea(0, 0, _phys_w, _phys_h);
  sendCommand(0x24);
  for (uint16_t y = 0; y < _phys_h; ++y) {
    const uint8_t* row = _fb_black + ((size_t)y * bytes_per_row);
    sendDataBlock(row, bytes_per_row);
  }
  sendCommand(0x22);
  sendData(0xCF);  // Partial update mode
  sendCommand(0x20);
  waitBusy();
}

void EPD_SSD1680::setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  // Align to byte boundaries
  x = (x / 8) * 8;
  w = ((w + 7) / 8) * 8;
  if (x + w > _phys_w) w = _phys_w - x;
  if (y + h > _phys_h) h = _phys_h - y;
  
  uint16_t x_end = x + w - 1;
  uint16_t y_end = y + h - 1;
  
  sendCommand(0x44); // SET_RAM_X_ADDRESS_START_END
  sendData(x / 8);
  sendData(x_end / 8);
  
  sendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END
  sendData(y & 0xFF);
  sendData((y >> 8) & 0xFF);
  sendData(y_end & 0xFF);
  sendData((y_end >> 8) & 0xFF);
  
  sendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
  sendData(x / 8);
  
  sendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
  sendData(y & 0xFF);
  sendData((y >> 8) & 0xFF);
}

void EPD_SSD1680::refreshPartial(bool isPartialMode) {
  if (isPartialMode) {
    sendCommand(0x22);  // DISPLAY_UPDATE_CONTROL_2
    sendData(0xC0);     // Partial update mode
  } else {
    sendCommand(0x22);
    sendData(0xF7);     // Full refresh
  }
  sendCommand(0x20);      // MASTER_ACTIVATION
  waitBusy();
}

void EPD_SSD1680::syncRectToRAM(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!_fb_black || w <= 0 || h <= 0) return;
  
  // Map logical rect to physical space
  int16_t xs, ys, xe, ye;
  mapRectToPhys(x, y, w, h, xs, ys, xe, ye);
  
  // Align to byte boundaries
  int xs_al = xs & ~7;
  int xe_al = xe | 7;
  if (xe_al >= (int)_phys_w) xe_al = _phys_w - 1;
  
  uint16_t win_x = xs_al;
  uint16_t win_y = ys;
  uint16_t win_w = xe_al - xs_al + 1;
  uint16_t win_h = ye - ys + 1;
  
  // Set partial window
  setPartialWindow(win_x, win_y, win_w, win_h);
  
  // Write only the window data to RAM
  const size_t bytes_per_row = (size_t)((_phys_w + 7) >> 3);
  for (int yy = ys; yy <= ye; ++yy) {
    const uint8_t* row = _fb_black + ((size_t)yy * bytes_per_row) + (xs_al >> 3);
    size_t nbytes = (size_t)((xe_al - xs_al + 1) >> 3);
    sendCommand(0x24); // WRITE_RAM_BW
    sendDataBlock(row, nbytes);
  }
  
  // Partial refresh
  refreshPartial(true);
}

void EPD_SSD1680::flushRect(int16_t x, int16_t y, int16_t w, int16_t h) {
  if (!_fb_black || w <= 0 || h <= 0) return;
  _Init_Part();
  int16_t xs, ys, xe, ye;
  mapRectToPhys(x, y, w, h, xs, ys, xe, ye);
  int16_t xs_al = xs & ~7;
  int16_t xe_al = xe | 7;
  // Устанавливаем частичную область как в GxEPD2 и пишем единым потоком
  uint16_t w_al = (uint16_t)(xe_al - xs_al + 1);
  _setPartialRamArea((uint16_t)xs_al, (uint16_t)ys, (uint16_t)w_al, (uint16_t)(ye - ys + 1));
  const size_t bytes_per_row_vis = (size_t)((_phys_w + 7) >> 3);
  size_t nbytes = (size_t)(w_al >> 3);
  // Ч/б плоскость
  sendCommand(0x24);
  for (int16_t yy = ys; yy <= ye; ++yy) {
    const uint8_t* row = _fb_black + ((size_t)yy * bytes_per_row_vis) + ((size_t)xs_al >> 3);
    sendDataBlock(row, nbytes);
  }
  // Цветная плоскость (инверсия), как в GxEPD2
  if (_cfg.variant != EPDVariant::BW && _fb_accent) {
    sendCommand(0x26);
    for (int16_t yy = ys; yy <= ye; ++yy) {
      const uint8_t* row2 = _fb_accent + ((size_t)yy * bytes_per_row_vis) + ((size_t)xs_al >> 3);
      for (size_t i=0;i<nbytes;++i) sendData(~row2[i]);
    }
  }
  issueUpdate(RefreshProfile::Partial);
}

void EPD_SSD1680::_InitDisplay() {
  // Полная инициализация как в GxEPD2_213_Z98c::_InitDisplay()
  // SW reset
  sendCommand(0x12);
  delay(10);
  // Driver output control (rows = _phys_h)
  sendCommand(0x01);
  sendData((_phys_h - 1) & 0xFF);
  sendData(((_phys_h - 1) >> 8) & 0x01);
  sendData(0x00);
  // Data entry mode: X+, Y+
  sendCommand(0x11); sendData(0x03);
  // Border waveform (как в GxEPD2)
  sendCommand(0x3C); sendData(0x05);
  // Built-in temperature sensor
  sendCommand(0x18); sendData(0x80);
  // Display update control 1
  sendCommand(0x21); sendData(0x00); sendData(0x80);
  // RAM area and counters (WIDTH = 128)
  const uint16_t ctrl_w = 128;
  sendCommand(0x44); sendData(0x00); sendData((ctrl_w / 8) - 1);
  sendCommand(0x45);
  sendData(0x00); sendData(0x00);
  sendData((_phys_h - 1) & 0xFF); sendData(((_phys_h - 1) >> 8) & 0x01);
  sendCommand(0x4E); sendData(0x00);
  sendCommand(0x4F); sendData(0x00); sendData(0x00);
}

void EPD_SSD1680::_Init_Full() {
  _InitDisplay();
  // For 3-color Z98c GxEPD2 doesn't load LUT explicitly. Skip LUT when variant != BW.
  if (_cfg.variant == EPDVariant::BW) {
    sendCommand(0x32);
    for (size_t i = 0; i < 30; ++i) {
      sendData(pgm_read_byte(&LUTDefault_full[i]));
    }
  }
}

void EPD_SSD1680::_Init_Part() {
  _InitDisplay();
  // For 3-color Z98c GxEPD2 doesn't load LUT explicitly. Skip LUT when variant != BW.
  if (_cfg.variant == EPDVariant::BW) {
    const uint8_t* lut = (_disp_mode == DisplayMode::Mode2) ? LUTFast_part : LUTDefault_part;
    sendCommand(0x32);
    for (size_t i = 0; i < 30; ++i) {
      sendData(pgm_read_byte(&lut[i]));
    }
  }
}

void EPD_SSD1680::_Update_Full() {
  // Display update control: full refresh flags (per GxEPD2 Z98c)
  sendCommand(0x22);
  sendData(0xF7);
  sendCommand(0x20);
  waitBusy();
  delay(20);
}

void EPD_SSD1680::_Update_Part() {
  // Display update control: partial refresh
  sendCommand(0x22);
  sendData(0xF7);
  sendCommand(0x20);
  waitBusy();
  // no extra delay
}

void EPD_SSD1680::_PowerOn() {
  // Как в GxEPD2_213_Z98c
  sendCommand(0x22);
  sendData(0xF8);
  sendCommand(0x20);
  waitBusy();
  delay(20);
}

void EPD_SSD1680::_PowerOff() {
  sendCommand(0x22);
  sendData(0x83); // VCOM off, power off
  sendCommand(0x20);
  waitBusy();
}

void EPD_SSD1680::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  sendCommand(0x44);
  sendData(x / 8);
  sendData((x + w - 1) / 8);
  sendCommand(0x45);
  sendData(y & 0xFF);
  sendData((y >> 8) & 0x01);
  uint16_t ye = y + h - 1;
  sendData(ye & 0xFF);
  sendData((ye >> 8) & 0x01);
  // Установить курсоры, как в GxEPD2
  sendCommand(0x4E);
  sendData(x / 8);
  sendCommand(0x4F);
  sendData(y & 0xFF);
  sendData((y >> 8) & 0x01);
}

// SSD1680 (Z98c в GxEPD2): BUSY=HIGH означает «занято», готовность = LOW.
// Поэтому ждём пока BUSY==HIGH и выходим, когда станет LOW.
void EPD_SSD1680::waitBusy() {
  if (_cfg.busy < 0) { delay(200); return; }
  unsigned long t0 = millis();
  while (digitalRead(_cfg.busy) == HIGH) {
    delay(1);
    if (millis() - t0 > 30000UL) break; // safety 30s
  }
}

// Low-level and helpers
void EPD_SSD1680::hwReset() {
  _io.hwReset();
  waitBusy();
}

void EPD_SSD1680::sendCommand(uint8_t c) { _io.sendCommand(c); }

void EPD_SSD1680::sendData(uint8_t d) { _io.sendData(d); }

void EPD_SSD1680::sendDataBlock(const uint8_t* p, size_t n) { _io.sendDataBlock(p, n); }

void EPD_SSD1680::setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  sendCommand(0x44); sendData(x0 >> 3); sendData(x1 >> 3);
  sendCommand(0x45);
  sendData(y0 & 0xFF); sendData((y0 >> 8) & 0x01);
  sendData(y1 & 0xFF); sendData((y1 >> 8) & 0x01);
  sendCommand(0x4E); sendData(x0 >> 3);
  sendCommand(0x4F);
  sendData(y0 & 0xFF); sendData((y0 >> 8) & 0x01);
}

void EPD_SSD1680::setCursor(int16_t, int16_t) {}

void EPD_SSD1680::issueUpdate(RefreshProfile p) {
  if (p == RefreshProfile::Full) {
    _Update_Full();
  } else {
    _Update_Part();
  }
}

void EPD_SSD1680::allocateBuffer() {
  freeBuffer();
  _fb_black_size = ((_phys_w + 7) / 8) * _phys_h;
  _fb_black = new uint8_t[_fb_black_size];
  if (_cfg.variant != EPDVariant::BW) {
    _fb_accent_size = _fb_black_size;
    _fb_accent = new uint8_t[_fb_accent_size];
  }
  // Инициализация: белый фон (ч/б=0xFF) и отсутствие цвета (акцент=0xFF, т.к. отправляем ~data)
  memset(_fb_black, 0xFF, _fb_black_size);
  if (_fb_accent) memset(_fb_accent, 0xFF, _fb_accent_size);
}

void EPD_SSD1680::freeBuffer() {
  if (_fb_black) { delete[] _fb_black; _fb_black = nullptr; _fb_black_size = 0; }
  if (_fb_accent) { delete[] _fb_accent; _fb_accent = nullptr; _fb_accent_size = 0; }
}

void EPD_SSD1680::mapXY(int16_t x, int16_t y, int16_t& xp, int16_t& yp) const {
  const int16_t w0 = _cfg.width;   // 122
  const int16_t h0 = _cfg.height;  // 250
  switch (_rotation) {
    case 0: xp = x;            yp = y;             break;           // 0°
    case 1: xp = w0 - 1 - y;   yp = x;             break;           // 90° CW
    case 2: xp = w0 - 1 - x;   yp = h0 - 1 - y;    break;           // 180°
    case 3: xp = y;            yp = h0 - 1 - x;    break;           // 270° CW
  }
}

void EPD_SSD1680::mapRectToPhys(int16_t x, int16_t y, int16_t w, int16_t h,
                                int16_t& xs, int16_t& ys, int16_t& xe, int16_t& ye) const {
  int16_t x1 = x, y1 = y, x2 = x + w - 1, y2 = y + h - 1, tmp;
  mapXY(x1, y1, xs, ys);
  mapXY(x2, y2, xe, ye);
  if (xs > xe) { tmp = xs; xs = xe; xe = tmp; }
  if (ys > ye) { tmp = ys; ys = ye; ye = tmp; }
  // Clamp to panel visible bounds
  if (xs < 0) xs = 0;
  if (ys < 0) ys = 0;
  if (xe >= _cfg.width)  xe = _cfg.width  - 1;
  if (ye >= _cfg.height) ye = _cfg.height - 1;
}

// ========== NEW: Full SSD1680 Command Implementation ==========

// --- Priority 1: Voltage Control ---

void EPD_SSD1680::setGateVoltage(uint8_t vgh_level) {
    sendCommand(0x03);
    sendData(vgh_level & 0x1F);
}

void EPD_SSD1680::setSourceVoltage(uint8_t vsh1, uint8_t vsh2, uint8_t vsl) {
    sendCommand(0x04);
    sendData(vsh1);
    sendData(vsh2);
    sendData(vsl);
}

void EPD_SSD1680::setVCOM(uint8_t vcom_level) {
    sendCommand(0x2C);
    sendData(vcom_level);
}

int16_t EPD_SSD1680::readTemperature() {
    uint8_t buf[2];
    _io.sendCommandReadBlock(0x35, buf, 2);
    int16_t temp = (int16_t)((buf[1] << 8) | buf[0]);
    return temp;
}

void EPD_SSD1680::setTemperature(int16_t temp) {
    sendCommand(0x37);
    sendData((uint8_t)(temp & 0xFF));
    sendData((uint8_t)((temp >> 8) & 0xFF));
}

// --- Priority 2: Advanced Waveform ---

void EPD_SSD1680::setBoosterSoftStart(uint8_t phase_a, uint8_t phase_b, uint8_t phase_c) {
    sendCommand(0x0C);
    sendData(phase_a);
    sendData(phase_b);
    sendData(phase_c);
}

void EPD_SSD1680::setGateLineWidth(uint8_t width) {
    sendCommand(0x15);
    sendData(width);
}

void EPD_SSD1680::setRegulatorControl(uint8_t ctrl) {
    sendCommand(0x18);
    sendData(ctrl);
}

void EPD_SSD1680::readRAM(uint16_t x, uint16_t y, uint8_t* buffer, size_t len) {
    if (!buffer || len == 0) return;
    sendCommand(0x4E);
    sendData((uint8_t)((x >> 3) & 0xFF));
    sendCommand(0x4F);
    sendData((uint8_t)(y & 0xFF));
    sendData((uint8_t)((y >> 8) & 0xFF));
    sendCommand(0x27);
    _io.readDataBlock(buffer, len);
}

void EPD_SSD1680::setBorderWaveform(uint8_t setting) {
    sendCommand(0x3C);
    sendData(setting & 0x03);
}

void EPD_SSD1680::setLUTEndOption(uint8_t option) {
    sendCommand(0x3F);
    sendData(option);
}

// --- Priority 3: Debug/OTP ---

void EPD_SSD1680::setPWMFrequency(uint8_t freq) {
    sendCommand(0x14);
    sendData(freq);
}

uint8_t EPD_SSD1680::readChipID() {
    return _io.sendCommandRead(0x1B);
}

uint8_t EPD_SSD1680::readVCOM() {
    return _io.sendCommandRead(0x2D);
}

void EPD_SSD1680::writeOTP(uint8_t addr, uint8_t data) {
    sendCommand(0x2E);
    sendData(addr);
    sendData(data);
}

uint8_t EPD_SSD1680::readOTP(uint8_t addr) {
    sendCommand(0x2F);
    sendData(addr);
    return _io.readData();
}

// --- Additional Commands ---

void EPD_SSD1680::setVCOMSenseDuration(uint8_t duration_sec) {
    sendCommand(0x29);
    sendData((duration_sec - 1) & 0x0F);
}

void EPD_SSD1680::setRAMContentOption(uint8_t option) {
    sendCommand(0x41);
    sendData(option);
}

bool EPD_SSD1680::checkHVReady() {
    sendCommand(0x22);
    sendData(0xC0);
    sendCommand(0x20);
    waitBusy();
    sendCommand(0x22);
    sendData(0x00);
    sendCommand(0x20);
    waitBusy();
    sendCommand(0x2F);
    sendData(0x00);
    uint8_t status = _io.readData();
    return (status & 0x20) == 0;
}

uint8_t EPD_SSD1680::performVCOMSense() {
    sendCommand(0x22);
    sendData(0xC0);
    sendCommand(0x20);
    waitBusy();
    sendCommand(0x22);
    sendData(0x01);
    sendCommand(0x20);
    waitBusy();
    return _io.sendCommandRead(0x2D);
}

void EPD_SSD1680::selectTemperatureSensor(bool internal) {
    sendCommand(0x18);
    sendData(internal ? 0x80 : 0x48);
}

void EPD_SSD1680::writeExternalTempSensor(uint8_t cmd) {
    sendCommand(0x22);
    sendData(0x48);
    sendCommand(0x20);
    waitBusy();
    sendCommand(0x18);
    sendData(cmd);
}

void EPD_SSD1680::enterDeepSleepMode1() {
    sendCommand(0x10);
    sendData(0x01);
    waitBusy();
}

void EPD_SSD1680::enterDeepSleepMode2() {
    sendCommand(0x10);
    sendData(0x11);
    waitBusy();
}

// --- Temperature-based LUT Auto-Selection ---

static const int16_t TEMP_RANGES_SSD1680[] = {0, 1000, 2500, 4000, 5000};

int8_t EPD_SSD1680::getTemperatureRange(int16_t temp) {
    if (temp < TEMP_RANGES_SSD1680[0]) return 0;
    if (temp < TEMP_RANGES_SSD1680[1]) return 1;
    if (temp < TEMP_RANGES_SSD1680[2]) return 2;
    if (temp < TEMP_RANGES_SSD1680[3]) return 3;
    if (temp < TEMP_RANGES_SSD1680[4]) return 4;
    return 5;
}

void EPD_SSD1680::enableAutoTempLUT(bool enable) {
    _auto_temp_lut = enable;
}

void EPD_SSD1680::setTempCallback(TempCallback callback) {
    _temp_callback = callback;
}

void EPD_SSD1680::checkAndApplyTempLUT() {
    if (!_auto_temp_lut) return;
    int16_t temp = readTemperature();
    _last_temp = temp;
    if (_temp_callback) {
        _temp_callback(temp);
    }
    setTemperature(temp);
    // Reload LUT from OTP based on temperature
    sendCommand(0x22);
    sendData(0xB1);
    sendCommand(0x20);
    waitBusy();
}

#endif // EINK_SELECTIVE_BUILD / EINK_ENABLE_SSD1680
