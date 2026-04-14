#pragma once
#include <Arduino.h>
#include <SPI.h>

// Supported controller types
enum class EPDController : uint8_t {
  SSD1681 = 0,  // Default controller
  SSD1680 = 1,
  IL3829 = 2,   // Not yet implemented
  SSD1608 = 3,  // Also used for UC8151
  UC8151 = 4    // Separate value for UC8151 (can be mapped to SSD1608 in factory)
};

// Display color variants
enum class EPDVariant : uint8_t {
  BW    = 0,   // Black/White
  BW_R  = 1,   // Black/White/Red (2R)
  BW_Y  = 2,   // Black/White/Yellow (3C)
  BW_RY = 3    // Black/White/Red/Yellow (4C)
};

// Display colors
enum class EPDColor : uint8_t {
  White  = 0,
  Black  = 1,
  Red    = 2,
  Yellow = 3
};

// Update profiles
enum class RefreshProfile : uint8_t { 
  Full    = 0, 
  Partial = 1 
};

// Display modes
enum class DisplayMode : uint8_t { 
  Mode1 = 1, 
  Mode2 = 2 
};

// Base configuration structure
struct EPDConfig {
  // Panel
  uint16_t width = 200;
  uint16_t height = 200;
  EPDVariant variant = EPDVariant::BW;
  EPDController controller = EPDController::SSD1681;
  uint8_t rotation = 0;

  // SPI pins (required for SW SPI; for HW SPI only cs/dc/rst/busy are mandatory)
  int8_t mosi = -1;
  int8_t sclk = -1;
  int8_t cs   = -1;
  int8_t dc   = -1;
  int8_t rst  = -1;
  int8_t busy = -1;

  // SPI options
  bool   use_hw_spi = true;
  bool   three_wire = false;
  bool   sw_spi_direct_gpio = true;
  bool   sw_spi_use_asm = false;
  uint32_t spi_hz = 4000000;

  // Debug
  bool   debug = false;
};

// Abstract base class for all EPD controllers
class EPD_Base {
public:
  virtual ~EPD_Base() = default;
  
  // Core functions
  virtual bool begin() = 0;
  virtual void end() = 0;
  virtual void sleep() = 0;
  virtual bool isBusy() const = 0;
  
  // Drawing functions
  virtual void setRotation(uint8_t r) = 0;
  virtual void drawPixel(int16_t x, int16_t y, EPDColor color) = 0;
  virtual void fillScreen(EPDColor color) = 0;
  
  // Display update functions
  virtual void flushFull() = 0;
  virtual void flushRect(int16_t x, int16_t y, int16_t w, int16_t h) = 0;
  
  // Framebuffer access
  virtual uint8_t* framebuffer() = 0;
  virtual size_t framebufferSize() const = 0;
  virtual uint8_t* framebufferAccent() = 0;  // For color layers
  virtual size_t framebufferAccentSize() const = 0;
  
  // Display-specific control
  virtual void setRefreshProfile(RefreshProfile p) = 0;
  virtual void setDisplayMode(DisplayMode m) = 0;
  // Text rendering helpers
  // Set additional spacing between characters in pixels (can be negative, zero, or positive)
  // Example: epd->setSterChar(1);
  virtual void setSterChar(int8_t px) { _char_spacing = px; }
  // Get current additional spacing between characters in pixels
  virtual int8_t getSterChar() const { return _char_spacing; }
  
  // Helper functions
  virtual uint16_t width() const = 0;
  virtual uint16_t height() const = 0;

protected:
  // Default storage for text spacing for all drivers
  int8_t _char_spacing = 0;
};

// Factory function to create appropriate EPD instance
EPD_Base* createEPD(const EPDConfig& cfg);
