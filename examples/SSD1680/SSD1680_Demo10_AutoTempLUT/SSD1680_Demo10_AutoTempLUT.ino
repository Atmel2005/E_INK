/**
 * SSD1680 Demo 10: Auto Temperature LUT Selection
 * Demonstrates automatic LUT selection based on temperature
 */

#include <SPI.h>
#include <EPD_Base.h>

#define EPD_CONTROLLER EPDController::SSD1680

#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7, PIN_SCLK = 6, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 6, PIN_SCLK = 4, PIN_CS = 7, PIN_DC = 5, PIN_RST = 2, PIN_BUSY = 3;
#else
  const int PIN_MOSI = 11, PIN_SCLK = 13, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 7;
#endif

EPDConfig cfg;
EPD_Base* epd = nullptr;
int16_t lastDisplayedTemp = -9999;

void onTempChange(int16_t temp) {
  Serial.print("Temperature changed: ");
  Serial.print(temp / 100.0);
  Serial.println(" C");
}

void drawTempGauge(int16_t x, int16_t y, int16_t radius, int16_t temp) {
  // Draw gauge background
  for (int angle = 180; angle <= 360; angle += 5) {
    float rad = angle * 3.14159 / 180;
    int px = x + cos(rad) * radius;
    int py = y + sin(rad) * radius;
    epd->drawPixel(px, py, EPDColor::Black);
    
    // Inner arc
    px = x + cos(rad) * (radius - 5);
    py = y + sin(rad) * (radius - 5);
    epd->drawPixel(px, py, EPDColor::Black);
  }
  
  // Draw tick marks
  for (int i = 0; i <= 5; i++) {
    int angle = 180 + i * 36;
    float rad = angle * 3.14159 / 180;
    for (int r = radius - 5; r < radius; r++) {
      int px = x + cos(rad) * r;
      int py = y + sin(rad) * r;
      epd->drawPixel(px, py, EPDColor::Black);
    }
  }
  
  // Draw needle
  int16_t constrainedTemp = constrain(temp, -1000, 6000);  // -10C to 60C
  int angle = 180 + map(constrainedTemp, -1000, 6000, 0, 180);
  float rad = angle * 3.14159 / 180;
  for (int r = 5; r < radius - 10; r++) {
    int px = x + cos(rad) * r;
    int py = y + sin(rad) * r;
    epd->drawPixel(px, py, EPDColor::Black);
  }
  
  // Center dot
  epd->drawPixel(x, y, EPDColor::Black);
  epd->drawPixel(x+1, y, EPDColor::Black);
  epd->drawPixel(x-1, y, EPDColor::Black);
  epd->drawPixel(x, y+1, EPDColor::Black);
  epd->drawPixel(x, y-1, EPDColor::Black);
}

void setup() {
  Serial.begin(115200);
  
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
  
  // Enable auto temperature LUT selection
  epd->setTempCallback(onTempChange);
  epd->enableAutoTempLUT(true);
  
  epd->setRotation(1);
  Serial.println("Auto Temperature LUT Demo");
  Serial.println("LUT will be automatically selected based on temperature");
}

void loop() {
  // Read current temperature
  int16_t temp = epd->readTemperature();
  
  // Only update if temperature changed significantly
  if (abs(temp - lastDisplayedTemp) > 50) {  // 0.5C change
    lastDisplayedTemp = temp;
    
    epd->fillScreen(EPDColor::White);
    
    // Draw temperature gauge
    int16_t cx = epd->width() / 2;
    int16_t cy = 60;
    drawTempGauge(cx, cy, 40, temp);
    
    // Draw temperature value
    int8_t range = epd->getTemperatureRange(temp);
    
    // Range indicator boxes
    const char* rangeLabels[] = {"<0", "0-10", "10-25", "25-40", "40-50", ">50"};
    int16_t boxY = 120;
    
    for (int i = 0; i < 6; i++) {
      int16_t boxX = 5 + i * 20;
      
      // Box outline
      for (int x = 0; x < 18; x++) {
        epd->drawPixel(boxX + x, boxY, EPDColor::Black);
        epd->drawPixel(boxX + x, boxY + 15, EPDColor::Black);
      }
      for (int y = 0; y < 15; y++) {
        epd->drawPixel(boxX, boxY + y, EPDColor::Black);
        epd->drawPixel(boxX + 17, boxY + y, EPDColor::Black);
      }
      
      // Highlight current range
      if (i == range) {
        for (int x = 1; x < 17; x++) {
          for (int y = 1; y < 14; y++) {
            epd->drawPixel(boxX + x, boxY + y, EPDColor::Black);
          }
        }
      }
    }
    
    // LUT status
    int16_t statusY = 150;
    for (int x = 0; x < 100; x++) {
      epd->drawPixel(10 + x, statusY, EPDColor::Black);
    }
    
    // Auto-LUT indicator
    if (epd->isAutoTempLUTEnabled()) {
      // Checkmark
      for (int i = 0; i < 10; i++) {
        epd->drawPixel(15 + i, statusY + 15 + i, EPDColor::Black);
        if (i < 7) epd->drawPixel(15 + i, statusY + 25 - i, EPDColor::Black);
      }
    }
    
    // VCOM Sense Duration setting
    epd->setVCOMSenseDuration(5);  // 5 seconds
    
    // RAM content option
    epd->setRAMContentOption(0x00);
    
    epd->flushFull();
    
    Serial.print("Temp: ");
    Serial.print(temp / 100.0);
    Serial.print(" C, Range: ");
    Serial.print(range);
    Serial.print(" (");
    Serial.print(rangeLabels[range]);
    Serial.println(")");
  }
  
  delay(5000);
}
