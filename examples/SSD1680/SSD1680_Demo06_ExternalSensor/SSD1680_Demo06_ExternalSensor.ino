/**
 * SSD1680 Demo 06: External I2C Temperature Sensor
 * Demonstrates communication with external I2C temperature sensor
 */

#include <SPI.h>
#include <EPD_Base.h>
#include <Wire.h>

#define EPD_CONTROLLER EPDController::SSD1680

#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7, PIN_SCLK = 6, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 11;
  const int PIN_SDA = 0, PIN_SCL = 1;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 6, PIN_SCLK = 4, PIN_CS = 7, PIN_DC = 5, PIN_RST = 2, PIN_BUSY = 3;
  const int PIN_SDA = 8, PIN_SCL = 9;
#else
  const int PIN_MOSI = 11, PIN_SCLK = 13, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 7;
  const int PIN_SDA = A4, PIN_SCL = A5;
#endif

EPDConfig cfg;
EPD_Base* epd = nullptr;

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  
  cfg.width = 122;
  cfg.height = 250;
  cfg.variant = EPDVariant::BW;
  cfg.controller = EPD_CONTROLLER;
  cfg.use_hw_spi = false;
  cfg.cs = PIN_CS; cfg.dc = PIN_DC; cfg.rst = PIN_RST;
  cfg.busy = PIN_BUSY; cfg.mosi = PIN_MOSI; cfg.sclk = PIN_SCLK;
  
  epd = createEPD(cfg);
  if (!epd || !epd->begin()) {
    Serial.println("EPD init failed!");
    while(1);
  }
  
  epd->setRotation(1);
  
  // Select external temperature sensor
  epd->selectTemperatureSensor(false);  // false = external I2C
  Serial.println("External I2C sensor selected");
}

void loop() {
  epd->fillScreen(EPDColor::White);
  
  // Draw sensor icon
  int16_t cx = 20, cy = 30;
  for (int i = 0; i < 8; i++) {
    epd->drawPixel(cx + i, cy, EPDColor::Black);
    epd->drawPixel(cx + i, cy + 15, EPDColor::Black);
    epd->drawPixel(cx, cy + i, EPDColor::Black);
    epd->drawPixel(cx + 7, cy + i, EPDColor::Black);
  }
  epd->drawPixel(cx + 3, cy + 5, EPDColor::Black);
  epd->drawPixel(cx + 4, cy + 5, EPDColor::Black);
  epd->drawPixel(cx + 3, cy + 10, EPDColor::Black);
  epd->drawPixel(cx + 4, cy + 10, EPDColor::Black);
  
  // Read from internal sensor for comparison
  int16_t internalTemp = epd->readTemperature();
  
  // Switch to external and try to read
  epd->selectTemperatureSensor(false);
  
  // Write command to typical I2C temp sensor (e.g., LM75)
  // This is sensor-specific - adjust for your sensor
  epd->writeExternalTempSensor(0x00);  // Temperature register
  
  // Read internal again for demo
  epd->selectTemperatureSensor(true);
  int16_t temp = epd->readTemperature();
  
  // Display temperature
  char buf[32];
  sprintf(buf, "Internal: %d.%02dC", temp/100, abs(temp%100));
  Serial.println(buf);
  
  // Draw temperature bars
  int16_t barY = 60;
  for (int i = 0; i < 100; i++) {
    epd->drawPixel(10 + i, barY, EPDColor::Black);
    epd->drawPixel(10 + i, barY + 10, EPDColor::Black);
  }
  int fill = map(constrain(temp, -1000, 6000), -1000, 6000, 0, 98);
  for (int i = 1; i < fill; i++) {
    for (int j = 1; j < 10; j++) {
      epd->drawPixel(11 + i, barY + j, EPDColor::Black);
    }
  }
  
  // Temperature range indicator
  int8_t range = epd->getTemperatureRange(temp);
  const char* ranges[] = {"<0C", "0-10C", "10-25C", "25-40C", "40-50C", ">50C"};
  
  // Draw range box
  int16_t boxX = 10, boxY = 90;
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 15; j++) {
      epd->drawPixel(boxX + i*16, boxY + j, EPDColor::Black);
      epd->drawPixel(boxX + i*16 + 14, boxY + j, EPDColor::Black);
    }
    for (int j = 0; j < 15; j++) {
      epd->drawPixel(boxX + i*16 + j, boxY, EPDColor::Black);
      epd->drawPixel(boxX + i*16 + j, boxY + 14, EPDColor::Black);
    }
    // Highlight current range
    if (i == range) {
      for (int x = 1; x < 14; x++) {
        for (int y = 1; y < 14; y++) {
          epd->drawPixel(boxX + i*16 + x, boxY + y, EPDColor::Black);
        }
      }
    }
  }
  
  epd->flushFull();
  delay(10000);
}
