#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1608)
#include "EPD_SSD1608.h"
#include <SPI.h>
#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/spi.h>
#endif

// SSD1608 Commands
#define SSD1608_DRIVER_CONTROL 0x01
#define SSD1608_GATE_VOLTAGE 0x03
#define SSD1608_SOURCE_VOLTAGE 0x04
#define SSD1608_DISPLAY_CONTROL 0x07
#define SSD1608_NON_OVERLAP 0x0B
#define SSD1608_BOOSTER_SOFT_START 0x0C
#define SSD1608_GATE_SCAN_START 0x0F
#define SSD1608_DEEP_SLEEP 0x10
#define SSD1608_DATA_MODE 0x11
#define SSD1608_SW_RESET 0x12
#define SSD1608_TEMP_SENSOR 0x18
#define SSD1608_TEMP_SENSOR_EN 0x1A
#define SSD1608_MASTER_ACTIVATION 0x20
#define SSD1608_DISPLAY_UPDATE_CTRL 0x21
#define SSD1608_WRITE_RAM_BW 0x24
#define SSD1608_WRITE_RAM_RED 0x26
#define SSD1608_READ_RAM 0x27
#define SSD1608_VCOM_SENSE 0x28
#define SSD1608_VCOM_DURATION 0x29
#define SSD1608_VCOM_OTP 0x2A
#define SSD1608_WRITE_VCOM 0x2C
#define SSD1608_READ_OTP 0x2D
#define SSD1608_WRITE_LUT 0x32
#define SSD1608_WRITE_DUMMY 0x3A
#define SSD1608_WRITE_GATELINE 0x3B
#define SSD1608_WRITE_BORDER 0x3C
#define SSD1608_SET_RAMXPOS 0x44
#define SSD1608_SET_RAMYPOS 0x45
#define SSD1608_SET_RAMXCOUNT 0x4E
#define SSD1608_SET_RAMYCOUNT 0x4F
#define SSD1608_NOP 0xFF

// Default LUT for full update
static const uint8_t lut_full_update[] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

// Default LUT for partial update
static const uint8_t lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Fast LUT for partial update (reduced transitions to minimize artifacts and speed up)
static const uint8_t lut_partial_update_fast[] = {
    0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x20, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

EPD_SSD1608::EPD_SSD1608(const EPDConfig& cfg) : _cfg(cfg) {
    _phys_w = cfg.width;
    _phys_h = cfg.height;
    _rotation = cfg.rotation;
}

EPD_SSD1608::~EPD_SSD1608() {
    freeBuffer();
}

bool EPD_SSD1608::begin() {
    // Unified IO initialization
    _io.begin();

    // Hardware reset
    hwReset();

    // Send initialization commands
    sendCommand(SSD1608_SW_RESET);
    waitBusy();

    // Set power settings
    sendCommand(SSD1608_DRIVER_CONTROL);
    sendData(0xD7);  // 215 gates
    sendData(0x00);  // 0
    sendData(0x00);  // 0

    // Set resolution and update mode
    setResolution();
    setLUTByHost(true);
    if (_profile == RefreshProfile::Full) {
        setLUT(lut_full_update);
    } else {
        const uint8_t* l = (_disp_mode == DisplayMode::Mode2) ? lut_partial_update_fast : lut_partial_update;
        setLUT(l);
    }

    // Power on
    powerOn();
    
    // Allocate framebuffer
    allocateBuffer();
    
    // Clear display
    fillScreen(EPDColor::White);
    flushFull();
    
    return true;
}

void EPD_SSD1608::end() {
    sleep();
    _io.end();
}

void EPD_SSD1608::sleep() {
    sendCommand(SSD1608_DEEP_SLEEP);
    sendData(0x01);  // Enter deep sleep mode
    delay(100);
}

bool EPD_SSD1608::isBusy() const {
    return digitalRead(_cfg.busy) == HIGH;
}

void EPD_SSD1608::setRotation(uint8_t r) {
    _rotation = r & 3;
    switch (_rotation) {
        case 0:
        case 2:
            _phys_w = _cfg.width;
            _phys_h = _cfg.height;
            break;
        case 1:
        case 3:
            _phys_w = _cfg.height;
            _phys_h = _cfg.width;
            break;
    }
}

void EPD_SSD1608::drawPixel(int16_t x, int16_t y, EPDColor color) {
    if (!inBounds(x, y)) return;
    
    int16_t xp = x, yp = y;
    mapXY(x, y, xp, yp);
    
    switch (color) {
        case EPDColor::Black:
            pixSet(_fb_black, xp, yp);
            break;
        case EPDColor::Red:
            if (_cfg.variant == EPDVariant::BW_R || _cfg.variant == EPDVariant::BW_RY) {
                pixSet(_fb_accent, xp, yp);
            }
            break;
        case EPDColor::Yellow:
            if (_cfg.variant == EPDVariant::BW_Y || _cfg.variant == EPDVariant::BW_RY) {
                pixSet(_fb_accent, xp, yp);
            }
            break;
        case EPDColor::White:
        default:
            pixClr(_fb_black, xp, yp);
            if (_cfg.variant != EPDVariant::BW) {
                pixClr(_fb_accent, xp, yp);
            }
            break;
    }
}

void EPD_SSD1608::fillScreen(EPDColor color) {
    uint8_t black_val = (color == EPDColor::Black || color == EPDColor::Red || color == EPDColor::Yellow) ? 0xFF : 0x00;
    memset(_fb_black, black_val, _fb_black_size);
    
    if (_cfg.variant != EPDVariant::BW) {
        uint8_t accent_val = ((color == EPDColor::Red && (_cfg.variant == EPDVariant::BW_R || _cfg.variant == EPDVariant::BW_RY)) ||
                             (color == EPDColor::Yellow && (_cfg.variant == EPDVariant::BW_Y || _cfg.variant == EPDVariant::BW_RY))) ? 0xFF : 0x00;
        memset(_fb_accent, accent_val, _fb_accent_size);
    }
}

void EPD_SSD1608::flushFull() {
    if (_fb_black == nullptr) return;
    
    setWindow(0, 0, _phys_w - 1, _phys_h - 1);
    
    // Send black/white buffer
    sendCommand(SSD1608_WRITE_RAM_BW);
    for (size_t i = 0; i < _fb_black_size; i++) {
        sendData(_fb_black[i]);
    }
    
    // Send red/yellow buffer if needed
    if (_cfg.variant != EPDVariant::BW) {
        sendCommand(SSD1608_WRITE_RAM_RED);
        for (size_t i = 0; i < _fb_accent_size; i++) {
            sendData(_fb_accent[i]);
        }
    }
    
    // Update display
    issueUpdate(RefreshProfile::Full);
}

void EPD_SSD1608::flushRect(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (_fb_black == nullptr) return;
    if (w <= 0 || h <= 0) return;

    // Map logical rect to physical coordinates (accounts for rotation)
    int16_t xs, ys, xe, ye;
    mapRectToPhys(x, y, w, h, xs, ys, xe, ye);

    // Align X to byte boundaries
    int16_t xs_al = xs & ~7;
    int16_t xe_al = xe | 7;
    if (xe_al >= _phys_w) xe_al = _phys_w - 1;

    // Program window once
    setWindow(xs_al, ys, xe_al, ye);

    // Write black/white plane for each line in window
    for (int16_t yy = ys; yy <= ye; ++yy) {
        // Set RAM address counters to beginning of line
        sendCommand(SSD1608_SET_RAMXCOUNT);
        sendData(xs_al >> 3);
        sendCommand(SSD1608_SET_RAMYCOUNT);
        sendData(yy & 0xFF);
        sendData((yy >> 8) & 0x01);

        // Write one row segment
        sendCommand(SSD1608_WRITE_RAM_BW);
        const uint8_t* row = _fb_black + (((size_t)yy * (size_t)_phys_w + (size_t)xs_al) >> 3);
        size_t nbytes = (size_t)((xe_al - xs_al + 1) >> 3);
        sendDataBlock(row, nbytes);
    }

    // Accent plane if present
    if (_cfg.variant != EPDVariant::BW && _fb_accent) {
        for (int16_t yy = ys; yy <= ye; ++yy) {
            sendCommand(SSD1608_SET_RAMXCOUNT);
            sendData(xs_al >> 3);
            sendCommand(SSD1608_SET_RAMYCOUNT);
            sendData(yy & 0xFF);
            sendData((yy >> 8) & 0x01);
            sendCommand(SSD1608_WRITE_RAM_RED);
            const uint8_t* row2 = _fb_accent + (((size_t)yy * (size_t)_phys_w + (size_t)xs_al) >> 3);
            size_t nbytes2 = (size_t)((xe_al - xs_al + 1) >> 3);
            sendDataBlock(row2, nbytes2);
        }
    }

    // Trigger partial refresh
    issueUpdate(RefreshProfile::Partial);
}

void EPD_SSD1608::hwReset() {
    _io.hwReset();
    waitBusy();
}

void EPD_SSD1608::waitBusy() {
    _io.waitBusy();
}

void EPD_SSD1608::sendCommand(uint8_t c) {
    _io.sendCommand(c);
}

void EPD_SSD1608::sendData(uint8_t d) {
    _io.sendData(d);
}

void EPD_SSD1608::sendDataBlock(const uint8_t* p, size_t n) {
    _io.sendDataBlock(p, n);
}

void EPD_SSD1608::setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    sendCommand(SSD1608_SET_RAMXPOS);
    sendData(x0 >> 3);
    sendData(x1 >> 3);
    
    sendCommand(SSD1608_SET_RAMYPOS);
    sendData(y0 & 0xFF);
    sendData((y0 >> 8) & 0x01);
    sendData(y1 & 0xFF);
    sendData((y1 >> 8) & 0x01);
    
    sendCommand(SSD1608_SET_RAMXCOUNT);
    sendData(x0 >> 3);
    
    sendCommand(SSD1608_SET_RAMYCOUNT);
    sendData(y0 & 0xFF);
    sendData((y0 >> 8) & 0x01);
}

void EPD_SSD1608::setCursor(int16_t x, int16_t y) {
    // Not used in this implementation
}

void EPD_SSD1608::issueUpdate(RefreshProfile p) {
    sendCommand(SSD1608_DISPLAY_UPDATE_CTRL);
    if (p == RefreshProfile::Full) {
        sendData(0xC7);  // Full update
    } else {
        sendData(0x0F);  // Partial update
    }
    
    sendCommand(SSD1608_MASTER_ACTIVATION);
    waitBusy();
}

void EPD_SSD1608::allocateBuffer() {
    freeBuffer();
    
    // Calculate buffer sizes
    _fb_black_size = ((_phys_w + 7) / 8) * _phys_h;
    _fb_black = new uint8_t[_fb_black_size];
    
    if (_cfg.variant != EPDVariant::BW) {
        _fb_accent_size = _fb_black_size;
        _fb_accent = new uint8_t[_fb_accent_size];
    }
    
    // Clear buffers
    memset(_fb_black, 0, _fb_black_size);
    if (_fb_accent) {
        memset(_fb_accent, 0, _fb_accent_size);
    }
}

void EPD_SSD1608::freeBuffer() {
    if (_fb_black) {
        delete[] _fb_black;
        _fb_black = nullptr;
        _fb_black_size = 0;
    }
    if (_fb_accent) {
        delete[] _fb_accent;
        _fb_accent = nullptr;
        _fb_accent_size = 0;
    }
}

void EPD_SSD1608::mapXY(int16_t x, int16_t y, int16_t& xp, int16_t& yp) const {
    switch (_rotation) {
        case 0:  // 0°
            xp = x;
            yp = y;
            break;
        case 1:  // 90°
            xp = _phys_w - y - 1;
            yp = x;
            break;
        case 2:  // 180°
            xp = _phys_w - x - 1;
            yp = _phys_h - y - 1;
            break;
        case 3:  // 270°
            xp = y;
            yp = _phys_h - x - 1;
            break;
    }
}

void EPD_SSD1608::mapRectToPhys(int16_t x, int16_t y, int16_t w, int16_t h,
                               int16_t& xs, int16_t& ys, int16_t& xe, int16_t& ye) const {
    int16_t x1 = x, y1 = y, x2 = x + w - 1, y2 = y + h - 1;
    int16_t tmp;
    
    mapXY(x1, y1, xs, ys);
    mapXY(x2, y2, xe, ye);
    
    if (xs > xe) {
        tmp = xs;
        xs = xe;
        xe = tmp;
    }
    
    if (ys > ye) {
        tmp = ys;
        ys = ye;
        ye = tmp;
    }
}

void EPD_SSD1608::setLUT(const uint8_t* lut) {
    sendCommand(SSD1608_WRITE_LUT);
    for (int i = 0; i < 30; i++) {
        sendData(lut[i]);
    }
}

void EPD_SSD1608::powerOn() {
    sendCommand(0x22);
    sendData(0xC0);
    sendCommand(0x20);
    waitBusy();
}

void EPD_SSD1608::powerOff() {
    sendCommand(0x22);
    sendData(0x83);
    sendCommand(0x20);
    waitBusy();
}

void EPD_SSD1608::setResolution() {
    sendCommand(0x61);  // Set resolution
    sendData(_phys_w);
    sendData((_phys_h >> 8) & 0x01);
    sendData(_phys_h & 0xFF);
}

void EPD_SSD1608::setLUTByHost(bool enabled) {
    sendCommand(0x22);
    sendData(enabled ? 0x40 : 0x00);
    sendCommand(0x20);
    waitBusy();
}

#endif // EINK_SELECTIVE_BUILD / EINK_ENABLE_SSD1608
