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
