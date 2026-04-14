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
  void syncToRAM();  // Quick update without clearing old frame

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

  // ========== NEW: Full SSD1680 Command Support ==========

  // --- Priority 1: Voltage Control ---
  void setGateVoltage(uint8_t vgh_level);  // 0x03
  void setSourceVoltage(uint8_t vsh1, uint8_t vsh2, uint8_t vsl);  // 0x04
  void setVCOM(uint8_t vcom_level);  // 0x2C
  int16_t readTemperature();  // 0x35
  void setTemperature(int16_t temp);  // 0x37

  // --- Priority 2: Advanced Waveform ---
  void setBoosterSoftStart(uint8_t phase_a, uint8_t phase_b, uint8_t phase_c);  // 0x0C
  void setGateLineWidth(uint8_t width);  // 0x15
  void setRegulatorControl(uint8_t ctrl);  // 0x18
  void readRAM(uint16_t x, uint16_t y, uint8_t* buffer, size_t len);  // 0x27
  void setBorderWaveform(uint8_t setting);  // 0x3C
  void setLUTEndOption(uint8_t option);  // 0x3F

  // --- Priority 3: Debug/OTP ---
  void setPWMFrequency(uint8_t freq);  // 0x14
  uint8_t readChipID();  // 0x1B
  uint8_t readVCOM();  // 0x2D
  void writeOTP(uint8_t addr, uint8_t data);  // 0x2E
  uint8_t readOTP(uint8_t addr);  // 0x2F

  // --- Additional Commands ---
  void setVCOMSenseDuration(uint8_t duration_sec);  // 0x29
  void setRAMContentOption(uint8_t option);  // 0x41
  bool checkHVReady();
  uint8_t performVCOMSense();
  void selectTemperatureSensor(bool internal);
  void writeExternalTempSensor(uint8_t cmd);
  void enterDeepSleepMode1();
  void enterDeepSleepMode2();

  // --- Temperature-based LUT Auto-Selection ---
  typedef void (*TempCallback)(int16_t temperature);
  void enableAutoTempLUT(bool enable);
  void setTempCallback(TempCallback callback);
  bool isAutoTempLUTEnabled() const { return _auto_temp_lut; }
  int8_t getTemperatureRange(int16_t temp);

private:
  // Config/state
  EPDConfig _cfg;
  EPD_IO _io{_cfg};
  uint16_t _phys_w = 0, _phys_h = 0;
  uint8_t _rotation = 0;
  RefreshProfile _profile = RefreshProfile::Full;
  DisplayMode _disp_mode = DisplayMode::Mode1;
  int8_t _char_spacing = 0; // additional spacing between characters in pixels

  // Auto temperature-LUT support
  bool _auto_temp_lut = false;
  TempCallback _temp_callback = nullptr;
  int16_t _last_temp = 2500;
  void checkAndApplyTempLUT();

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

  // Helper for read operations
  uint8_t readData() { return _io.readData(); }
  void readDataBlock(uint8_t* buf, size_t n) { _io.readDataBlock(buf, n); }

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
