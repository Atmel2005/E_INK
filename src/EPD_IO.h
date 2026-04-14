#pragma once
#include "EPD_Base.h"
#include <SPI.h>

// Unified IO layer for DC/CS/RST/BUSY and SPI transfers
// Supports both write and read operations for SSD1681 commands
class EPD_IO {
public:
  explicit EPD_IO(const EPDConfig& cfg) : _cfg(cfg) {}

  void begin() {
    if (_cfg.use_hw_spi && !_cfg.three_wire) {
      _spi = &SPI;
      _spi->begin();
    } else {
      pinMode(_cfg.sclk, OUTPUT);
      pinMode(_cfg.mosi, OUTPUT);
      digitalWrite(_cfg.sclk, LOW);
      digitalWrite(_cfg.mosi, LOW);
    }
    pinMode(_cfg.cs, OUTPUT);
    if (!_cfg.three_wire && _cfg.dc >= 0) pinMode(_cfg.dc, OUTPUT);
    if (_cfg.rst >= 0) pinMode(_cfg.rst, OUTPUT);
    if (_cfg.busy >= 0) pinMode(_cfg.busy, INPUT);
    digitalWrite(_cfg.cs, HIGH);
    if (!_cfg.three_wire && _cfg.dc >= 0) digitalWrite(_cfg.dc, HIGH);
    if (_cfg.rst >= 0) digitalWrite(_cfg.rst, HIGH);
#if !defined(ARDUINO_ARCH_AVR)
    if (_cfg.use_hw_spi && !_cfg.three_wire) _spi->setFrequency(_cfg.spi_hz);
#endif
  }

  void end() {
    if (_cfg.use_hw_spi && _spi) _spi->end();
  }

  void hwReset() {
    if (_cfg.rst < 0) return;
    digitalWrite(_cfg.rst, LOW);
    delay(10);
    digitalWrite(_cfg.rst, HIGH);
    delay(10);
  }

  void waitBusy() const {
    if (_cfg.busy < 0) return;
    while (digitalRead(_cfg.busy) == HIGH) delay(1);
  }

  void sendCommand(uint8_t c) {
    if (_cfg.three_wire) {
      // 3-wire: send D/C=0 bit followed by 8 data bits
      digitalWrite(_cfg.cs, LOW);
      sw_write_dc_bit(false);
      sw_write_byte(c);
      digitalWrite(_cfg.cs, HIGH);
      return;
    }
    if (_cfg.dc >= 0) digitalWrite(_cfg.dc, LOW);
    digitalWrite(_cfg.cs, LOW);
    if (_cfg.use_hw_spi) _spi->transfer(c);
    else shiftOut(_cfg.mosi, _cfg.sclk, MSBFIRST, c);
    digitalWrite(_cfg.cs, HIGH);
  }

  void sendData(uint8_t d) {
    if (_cfg.three_wire) {
      // 3-wire: send D/C=1 bit followed by 8 data bits
      digitalWrite(_cfg.cs, LOW);
      sw_write_dc_bit(true);
      sw_write_byte(d);
      digitalWrite(_cfg.cs, HIGH);
      return;
    }
    if (_cfg.dc >= 0) digitalWrite(_cfg.dc, HIGH);
    digitalWrite(_cfg.cs, LOW);
    if (_cfg.use_hw_spi) _spi->transfer(d);
    else shiftOut(_cfg.mosi, _cfg.sclk, MSBFIRST, d);
    digitalWrite(_cfg.cs, HIGH);
  }

  void sendDataBlock(const uint8_t* p, size_t n) {
    if (_cfg.three_wire) {
      if (!p || n==0) return;
      digitalWrite(_cfg.cs, LOW);
      for (size_t i=0;i<n;++i) {
        sw_write_dc_bit(true);
        sw_write_byte(p[i]);
      }
      digitalWrite(_cfg.cs, HIGH);
      return;
    }
    if (_cfg.dc >= 0) digitalWrite(_cfg.dc, HIGH);
    digitalWrite(_cfg.cs, LOW);
    if (_cfg.use_hw_spi) {
      for (size_t i=0;i<n;++i) _spi->transfer(p[i]);
    } else {
      for (size_t i=0;i<n;++i) shiftOut(_cfg.mosi, _cfg.sclk, MSBFIRST, p[i]);
    }
    digitalWrite(_cfg.cs, HIGH);
  }

  // ========== READ OPERATIONS (for SSD1681 commands 0x1B, 0x27, 0x2D, 0x2F, 0x35) ==========

  // Read single byte after sending command (CS must already be LOW from command)
  uint8_t readData() {
    if (_cfg.three_wire) {
      // 3-wire SPI: D/C=1, dummy write, then read 8 bits
      sw_write_dc_bit(true);
      sw_write_byte(0x00);  // Dummy write
      uint8_t val = sw_read_byte();
      return val;
    }
    // 4-wire SPI: D/C=HIGH, dummy write, read byte
    if (_cfg.dc >= 0) digitalWrite(_cfg.dc, HIGH);
    uint8_t val;
    if (_cfg.use_hw_spi) {
      _spi->transfer(0x00);  // Dummy write
      val = _spi->transfer(0x00);
    } else {
      sw_write_byte(0x00);  // Dummy write (MOSI output)
      val = sw_read_byte();
    }
    return val;
  }

  // Read multiple bytes after command (CS must already be LOW from command)
  void readDataBlock(uint8_t* buf, size_t n) {
    if (!buf || n == 0) return;
    if (_cfg.three_wire) {
      sw_write_dc_bit(true);
      sw_write_byte(0x00);  // Dummy write
      for (size_t i = 0; i < n; ++i) {
        buf[i] = sw_read_byte();
      }
      return;
    }
    // 4-wire: DC HIGH for data, then dummy write + reads
    if (_cfg.dc >= 0) digitalWrite(_cfg.dc, HIGH);
    if (_cfg.use_hw_spi) {
      _spi->transfer(0x00);  // Dummy write
      for (size_t i = 0; i < n; ++i) {
        buf[i] = _spi->transfer(0x00);
      }
    } else {
      sw_write_byte(0x00);  // Dummy write (MOSI output)
      for (size_t i = 0; i < n; ++i) {
        buf[i] = sw_read_byte();
      }
    }
  }

  // Send command then read data (CS stays LOW between command and read)
  uint8_t sendCommandRead(uint8_t cmd) {
    // Send command with CS LOW
    if (_cfg.three_wire) {
      digitalWrite(_cfg.cs, LOW);
      sw_write_dc_bit(false);
      sw_write_byte(cmd);
    } else {
      if (_cfg.dc >= 0) digitalWrite(_cfg.dc, LOW);
      digitalWrite(_cfg.cs, LOW);
      if (_cfg.use_hw_spi) _spi->transfer(cmd);
      else shiftOut(_cfg.mosi, _cfg.sclk, MSBFIRST, cmd);
    }
    // Now read data (CS stays LOW)
    uint8_t val = readData();
    digitalWrite(_cfg.cs, HIGH);
    return val;
  }

  // Send command then read multiple bytes (CS stays LOW)
  void sendCommandReadBlock(uint8_t cmd, uint8_t* buf, size_t n) {
    // Send command with CS LOW
    if (_cfg.three_wire) {
      digitalWrite(_cfg.cs, LOW);
      sw_write_dc_bit(false);
      sw_write_byte(cmd);
    } else {
      if (_cfg.dc >= 0) digitalWrite(_cfg.dc, LOW);
      digitalWrite(_cfg.cs, LOW);
      if (_cfg.use_hw_spi) _spi->transfer(cmd);
      else shiftOut(_cfg.mosi, _cfg.sclk, MSBFIRST, cmd);
    }
    // Now read data (CS stays LOW)
    readDataBlock(buf, n);
    digitalWrite(_cfg.cs, HIGH);
  }

private:
  EPDConfig _cfg;
  SPIClass* _spi = nullptr;

  inline void sw_clk_low() { digitalWrite(_cfg.sclk, LOW); }
  inline void sw_clk_high() { digitalWrite(_cfg.sclk, HIGH); }
  inline void sw_mosi_set(uint8_t v) { digitalWrite(_cfg.mosi, v ? HIGH : LOW); }
  inline void sw_write_dc_bit(bool isData) {
    // mode0 style: sample on rising edge
    sw_clk_low();
    sw_mosi_set(isData ? 1 : 0);
    sw_clk_high();
  }
  inline void sw_write_byte(uint8_t b) {
    for (int8_t i=7;i>=0;--i) {
      sw_clk_low();
      sw_mosi_set((b >> i) & 1);
      sw_clk_high();
    }
  }

  // Software SPI Read byte (bidirectional MOSI - switch to INPUT for reading)
  inline uint8_t sw_read_byte() {
    uint8_t val = 0;
    // Switch MOSI to input mode (bidirectional)
    pinMode(_cfg.mosi, INPUT);
    for (int8_t i = 7; i >= 0; --i) {
      sw_clk_low();
      delayMicroseconds(1);
      sw_clk_high();
      if (digitalRead(_cfg.mosi) == HIGH) {
        val |= (1 << i);
      }
    }
    // Switch MOSI back to output
    pinMode(_cfg.mosi, OUTPUT);
    return val;
  }
};
