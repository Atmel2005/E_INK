#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "EPD_Base.h"
#include "EPD_IO.h"

// Minimal SSD1681 200x200 E-Paper driver (B/W first). Supports HW and SW SPI.
// Target MCUs: RP2040, ESP32 family, STM32, ATmega328 (UNO).

class EPD_SSD1681 : public EPD_Base {
public:
    explicit EPD_SSD1681(const EPDConfig& cfg);
    ~EPD_SSD1681();
    
    // Core functions
    bool begin() override;
    void end() override;
    void sleep() override;
    bool isBusy() const override;
    
    // Drawing functions
    void setRotation(uint8_t r) override;
    void drawPixel(int16_t x, int16_t y, EPDColor color) override;
    void fillScreen(EPDColor color) override;
    
    // Display update functions
    void flushFull() override;
    void flushRect(int16_t x, int16_t y, int16_t w, int16_t h) override;
    void updatePart(int16_t x, int16_t y, int16_t w, int16_t h);
    void syncToRAM();  // Write framebuffer to RAM without triggering display update
    
    // Framebuffer access
    uint8_t* framebuffer() override;
    size_t framebufferSize() const override;
    uint8_t* framebufferAccent() override;
    size_t framebufferAccentSize() const override;
    
    // Display control
    void setRefreshProfile(RefreshProfile p) override;
    void setDisplayMode(DisplayMode m) override;
    
    // Helper functions
    uint16_t width() const override { return (_rotation & 1) ? _phys_h : _phys_w; }
    uint16_t height() const override { return (_rotation & 1) ? _phys_w : _phys_h; }
    
    // Backward compatibility
    void drawPixel(int16_t x, int16_t y, uint8_t color) {
        drawPixel(x, y, color ? EPDColor::Black : EPDColor::White);
    }
    
    void fillScreen(uint8_t color) {
        fillScreen(color ? EPDColor::Black : EPDColor::White);
    }
    
    // Additional methods for backward compatibility
    void setUpdateControlFlags(uint8_t full_flags, uint8_t partial_flags);
    void reloadTempAndLUT(bool mode2 = false);
    void reloadLUTOnly(bool mode2 = false);
    void setDisplayUpdateControl1(uint8_t A, uint8_t B);

    // ========== NEW: Full SSD1681 Command Support (15 previously missing commands) ==========

    // --- Priority 1: Voltage Control ---
    // 0x03: Gate Driving Voltage (VGH/VGL)
    void setGateVoltage(uint8_t vgh_level);  // VGH = 10V + level*0.5V (0-20 = 10V-20V)
    // 0x04: Source Driving Voltage (VSH1, VSH2, VSL)
    void setSourceVoltage(uint8_t vsh1_level, uint8_t vsh2_level, uint8_t vsl_level);
    // 0x2C: Write VCOM DC level
    void setVCOM(uint8_t vcom_level);  // DCVCOM = -0.2V + level*0.02V (0-140 = -0.2V to -3.0V)
    // 0x35: Read internal temperature sensor
    int16_t readTemperature();  // Returns temperature in 0.01°C units
    // 0x37: Write temperature for LUT selection
    void setTemperature(int16_t temp);  // Set temperature in 0.01°C units

    // --- Priority 2: Advanced Waveform ---
    // 0x0C: Booster Soft Start
    void setBoosterSoftStart(uint8_t phase_a, uint8_t phase_b, uint8_t phase_c);
    // 0x15: Gate Line Width
    void setGateLineWidth(uint8_t width);
    // 0x18: Regulator Control
    void setRegulatorControl(uint8_t ctrl);
    // 0x27: Read RAM (B/W)
    void readRAM(uint16_t x, uint16_t y, uint8_t* buffer, size_t len);
    // 0x3C: Border Waveform Control
    void setBorderWaveform(uint8_t setting);  // 0=VSS, 1=VSH1, 2=VCOM
    // 0x3F: LUT End Option
    void setLUTEndOption(uint8_t option);

    // --- Priority 3: Debug/OTP ---
    // 0x14: PWM Frequency Control
    void setPWMFrequency(uint8_t freq);
    // 0x1B: Read Chip ID
    uint8_t readChipID();
    // 0x2D: Read VCOM Register
    uint8_t readVCOM();
    // 0x2E: Write OTP (permanent!)
    void writeOTP(uint8_t addr, uint8_t data);
    // 0x2F: Read OTP
    uint8_t readOTP(uint8_t addr);

    // ========== Additional Commands Found in Datasheet ==========

    // 0x29: VCOM Sense Duration
    // Sets VCOM sensing duration: duration = (duration_sec-1) seconds (1-16 sec)
    void setVCOMSenseDuration(uint8_t duration_sec);

    // 0x41: RAM Content Option for Display Update
    // Selects RAM content option (affects read operations)
    void setRAMContentOption(uint8_t option);

    // HV Ready Detection (via 0x22)
    // Detects if analog block is ready, returns true if ready
    bool checkHVReady();

    // VCOM Sense (via 0x22)
    // Performs VCOM sensing, returns sensed VCOM value
    uint8_t performVCOMSense();

    // External Temperature Sensor Control (via 0x22)
    // Select internal (true) or external I2C sensor (false)
    void selectTemperatureSensor(bool internal);

    // Write command to external I2C temperature sensor
    void writeExternalTempSensor(uint8_t cmd);

    // Deep Sleep Mode selection
    // Mode 1: Lower power, Mode 2: Even lower power
    void enterDeepSleepMode1();
    void enterDeepSleepMode2();

    // ========== Temperature-based LUT Auto-Selection ==========
    
    // Callback type for temperature notification
    typedef void (*TempCallback)(int16_t temperature);
    
    // Enable/disable automatic LUT selection based on temperature
    // When enabled, temperature is read before each update and appropriate LUT is loaded
    void enableAutoTempLUT(bool enable);
    
    // Set temperature change callback
    // Callback is invoked when temperature is read during auto-LUT mode
    void setTempCallback(TempCallback callback);
    
    // Get current auto-temp-LUT state
    bool isAutoTempLUTEnabled() const { return _auto_temp_lut; }
    
    // Manual temperature range check (for external use)
    int8_t getTemperatureRange(int16_t temp);  // Returns range index 0-5
    
private:
    // Config/state
    EPDConfig _cfg;
    EPD_IO _io{_cfg};
    uint16_t _phys_w = 0, _phys_h = 0;
    uint8_t _rotation = 0;
    RefreshProfile _profile = RefreshProfile::Full;
    DisplayMode _disp_mode = DisplayMode::Mode1;
    uint8_t _upd_full_flags = 0xC7;    // DISPLAY_UPDATE_CONTROL_2 flags for full
    uint8_t _upd_partial_flags = 0xC7; // default same as full
    
    // Auto temperature-LUT support
    bool _auto_temp_lut = false;
    TempCallback _temp_callback = nullptr;
    int16_t _last_temp = 2500;  // Last read temperature (default 25°C)
    
    // Internal helper for auto temp LUT
    void checkAndApplyTempLUT();
    
    // Framebuffers
    uint8_t* _fb_black = nullptr;
    size_t _fb_black_size = 0;
    uint8_t* _fb_accent = nullptr;
    size_t _fb_accent_size = 0;
    
    // Internal methods
    void hwReset();
    void waitBusy();
    void sendCommand(uint8_t c);
    void sendData(uint8_t d);
    void sendDataBlock(const uint8_t* p, size_t n);
    void setWindow(int x0, int y0, int x1, int y1);
    void setPointer(int x, int y);
    void issueUpdate(RefreshProfile p);
    void setLUTForPanel(RefreshProfile profile);
    
    // Inline helpers
    inline bool inBounds(int16_t x, int16_t y) const {
        return (x >= 0 && y >= 0 && 
                static_cast<uint16_t>(x) < width() && 
                static_cast<uint16_t>(y) < height());
    }
    
    inline void pixSet(uint8_t* fb, int16_t x, int16_t y) { 
        fb[(size_t)(x + (y * _phys_w)) >> 3] |= (0x80u >> (x & 7)); 
    }
    
    inline void pixClr(uint8_t* fb, int16_t x, int16_t y) { 
        fb[(size_t)(x + (y * _phys_w)) >> 3] &= ~(0x80u >> (x & 7)); 
    }
    
    void mapXY(int16_t x, int16_t y, int16_t& xp, int16_t& yp) const;
    void mapRectToPhys(int16_t x, int16_t y, int16_t w, int16_t h,
                     int& xs, int& ys, int& xe, int& ye) const;
    void allocateBuffer();
    void freeBuffer();

    // Helper for read operations
    uint8_t readData() { return _io.readData(); }
    void readDataBlock(uint8_t* buf, size_t n) { _io.readDataBlock(buf, n); }
};

// Backward compatibility for existing code
class EPD_SSD1681_Compat : public EPD_SSD1681 {
public:
    explicit EPD_SSD1681_Compat(const EPDConfig& cfg) : EPD_SSD1681(cfg) {}
    
    // Explicitly bring base class methods into scope
    using EPD_SSD1681::drawPixel;
    using EPD_SSD1681::fillScreen;
    
    // Compatibility methods
    void drawPixel(int16_t x, int16_t y, uint8_t color) {
        EPD_SSD1681::drawPixel(x, y, color);
    }
    
    void fillScreen(uint8_t color) {
        EPD_SSD1681::fillScreen(color);
    }
};
