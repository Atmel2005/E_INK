#pragma once
#include "EPD_Base.h"
#include <SPI.h>

// Unified IO layer for DC/CS/RST/BUSY and SPI transfers
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
};
