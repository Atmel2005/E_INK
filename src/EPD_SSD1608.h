#pragma once
#include "EPD_Base.h"
#include "EPD_IO.h"

class EPD_SSD1608 : public EPD_Base {
public:
    explicit EPD_SSD1608(const EPDConfig& cfg);
    ~EPD_SSD1608() override;

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

    // Framebuffer access
    uint8_t* framebuffer() override { return _fb_black; }
    size_t framebufferSize() const override { return _fb_black_size; }
    uint8_t* framebufferAccent() override { return _fb_accent; }
    size_t framebufferAccentSize() const override { return _fb_accent_size; }

    // Display control
    void setRefreshProfile(RefreshProfile p) override { _profile = p; }
    void setDisplayMode(DisplayMode m) override { _disp_mode = m; }

    // Helper functions
    uint16_t width() const override { return (_rotation & 1) ? _phys_h : _phys_w; }
    uint16_t height() const override { return (_rotation & 1) ? _phys_w : _phys_h; }

private:
    // Configuration and state
    EPDConfig _cfg;
    EPD_IO _io{_cfg};
    uint16_t _phys_w = 0, _phys_h = 0;
    uint8_t _rotation = 0;
    RefreshProfile _profile = RefreshProfile::Full;
    DisplayMode _disp_mode = DisplayMode::Mode1;

    // Framebuffers
    uint8_t* _fb_black = nullptr;
    size_t _fb_black_size = 0;
    uint8_t* _fb_accent = nullptr;
    size_t _fb_accent_size = 0;

    // SPI via EPD_IO

    // Low-level functions
    void hwReset();
    void waitBusy();
    void sendCommand(uint8_t c);
    void sendData(uint8_t d);
    void sendDataBlock(const uint8_t* p, size_t n);
    void setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    void setCursor(int16_t x, int16_t y);
    void issueUpdate(RefreshProfile p);
    
    // Buffer management
    void allocateBuffer();
    void freeBuffer();

    // Helper functions
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
                      int16_t& xs, int16_t& ys, int16_t& xe, int16_t& ye) const;

    // SSD1608 specific commands
    void setLUT(const uint8_t* lut);
    void powerOn();
    void powerOff();
    void setResolution();
    void setLUTByHost(bool enabled);
};
