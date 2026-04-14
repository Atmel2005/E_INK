# E_INK Library

Unified driver for e-paper displays (EPD) with support for multiple controllers and SPI backends.

## Features

- **Multiple controllers**: SSD1680, SSD1681, SSD1608, UC8151, IL3829
- **Color variants**: Black/White, Black/White/Red, Black/White/Yellow, Black/White/Red/Yellow
- **SPI modes**: Hardware SPI, Software SPI, 3-wire and 4-wire
- **Refresh modes**: Full refresh and partial update
- **Rotation support**: 0°, 90°, 180°, 270°
- **Multi-platform**: RP2040, ESP32, STM32, AVR (Arduino Uno)

## Installation

1. Download or clone this repository
2. Copy the `E_INK` folder to your Arduino libraries directory:
   - Windows: `Documents\Arduino\libraries\`
   - macOS/Linux: `~/Arduino/libraries/`
3. Restart Arduino IDE

## Quick Start

```cpp
#include <E_INK.h>

EPDConfig cfg;
EPD_Base* epd = nullptr;

void setup() {
  // Configure display
  cfg.width = 200;
  cfg.height = 200;
  cfg.controller = EPDController::SSD1681;
  cfg.variant = EPDVariant::BW;
  
  // SPI pins (adjust for your board)
  cfg.cs = 10;
  cfg.dc = 9;
  cfg.rst = 8;
  cfg.busy = 7;
  cfg.use_hw_spi = true;
  
  // Create and initialize display
  epd = createEPD(cfg);
  epd->begin();
  
  // Draw content
  epd->fillScreen(EPDColor::White);
  epd->drawPixel(100, 100, EPDColor::Black);
  epd->flushFull();
  
  // Sleep to save power
  epd->sleep();
}

void loop() {}
```

## Supported Controllers

| Controller | Resolution | Colors | Notes |
|------------|------------|--------|-------|
| SSD1680 | 122×250 | BW, BW_R | Common 2.13" displays |
| SSD1681 | 200×200 | BW, BW_R, BW_Y | 1.54" displays |
| SSD1608 | 200×200 | BW | Older 1.54" displays |
| UC8151 | Various | BW | Low-voltage displays |
| IL3829 | Various | BW | GDEH0213B73 displays |

## Configuration Options

```cpp
EPDConfig cfg;

// Display dimensions
cfg.width = 200;
cfg.height = 200;

// Controller and variant
cfg.controller = EPDController::SSD1681;
cfg.variant = EPDVariant::BW_R;  // BW, BW_R, BW_Y, BW_RY

// SPI pins
cfg.mosi = 11;  // SW SPI only
cfg.sclk = 13;  // SW SPI only
cfg.cs   = 10;
cfg.dc   = 9;
cfg.rst  = 8;
cfg.busy = 7;

// SPI options
cfg.use_hw_spi = true;   // Use hardware SPI
cfg.three_wire = false;  // 3-wire SPI mode
cfg.spi_hz = 4000000;    // SPI clock speed

// Rotation
cfg.rotation = 0;  // 0, 1, 2, 3 (0°, 90°, 180°, 270°)
```

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `begin()` | Initialize display |
| `end()` | Deinitialize display |
| `sleep()` | Enter deep sleep mode |
| `isBusy()` | Check if display is busy |

### Drawing Functions

| Function | Description |
|----------|-------------|
| `drawPixel(x, y, color)` | Draw a single pixel |
| `fillScreen(color)` | Fill entire screen with color |
| `setRotation(r)` | Set rotation (0-3) |

### Display Update

| Function | Description |
|----------|-------------|
| `flushFull()` | Full display refresh |
| `flushRect(x, y, w, h)` | Partial refresh of rectangle |

### Framebuffer Access

| Function | Description |
|----------|-------------|
| `framebuffer()` | Get pointer to black/white buffer |
| `framebufferSize()` | Get buffer size in bytes |
| `framebufferAccent()` | Get pointer to color buffer (red/yellow) |

### Display Control

| Function | Description |
|----------|-------------|
| `setRefreshProfile(profile)` | Set Full or Partial refresh mode |
| `setDisplayMode(mode)` | Set Mode1 or Mode2 |
| `setSterChar(px)` | Set extra character spacing |
| `width()` / `height()` | Get display dimensions |

## Colors

```cpp
EPDColor::White   // White pixel
EPDColor::Black   // Black pixel
EPDColor::Red     // Red pixel (BW_R variant)
EPDColor::Yellow  // Yellow pixel (BW_Y variant)
```

## Examples

- **EPD_SSD1681_Intro** - Basic text display with font rendering
- **EPD_SSD1680_2R_Demo** - Red/Black/White display demo
- **Chasy_RBW** - Clock application with partial refresh

## Pin Connections

### RP2040 (Raspberry Pi Pico)
| Display Pin | GPIO |
|-------------|------|
| MOSI | 7 |
| SCLK | 6 |
| CS | 10 |
| DC | 9 |
| RST | 8 |
| BUSY | 11 |

### ESP32
| Display Pin | GPIO |
|-------------|------|
| MOSI | 23 |
| SCLK | 18 |
| CS | 5 |
| DC | 17 |
| RST | 16 |
| BUSY | 4 |

### Arduino Uno
| Display Pin | GPIO |
|-------------|------|
| MOSI | 11 |
| SCLK | 13 |
| CS | 10 |
| DC | 9 |
| RST | 8 |
| BUSY | 7 |

## License

MIT License

## Author

ATMEL (atmel2005)

## Repository

https://github.com/atmel2005/E_INK
