/**
 * SSD1681 Demo 10: Auto Temperature LUT Selection
 * Automatic LUT selection based on temperature
 * Resolution: 200x200
 */

#include <SPI.h>
#include <EPD_Base.h>

#define EPD_CONTROLLER EPDController::SSD1681

#if defined(ARDUINO_ARCH_RP2040)
  const int PIN_MOSI = 7, PIN_SCLK = 6, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 11;
#elif defined(ARDUINO_ARCH_ESP32)
  const int PIN_MOSI = 6, PIN_SCLK = 4, PIN_CS = 7, PIN_DC = 5, PIN_RST = 2, PIN_BUSY = 3;
#else
  const int PIN_MOSI = 11, PIN_SCLK = 13, PIN_CS = 10, PIN_DC = 9, PIN_RST = 8, PIN_BUSY = 7;
#endif

EPDConfig cfg;
EPD_Base* epd = nullptr;
int16_t lastTemp = -9999;

void onTempChange(int16_t temp) {
  Serial.print("Temp callback: ");
  Serial.print(temp / 100.0);
  Serial.println(" C");
}

void drawGauge(int16_t cx, int16_t cy, int16_t radius, int16_t temp) {
  // Background arc
  for (int angle = 135; angle <= 405; angle += 3) {
    float rad = angle * 3.14159 / 180;
    int x = cx + cos(rad) * radius;
    int y = cy + sin(rad) * radius;
    epd->drawPixel(x, y, EPDColor::Black);
  }
  
  // Fill arc based on temp
  int16_t constrained = constrain(temp, -1000, 6000);
  int fillAngle = 135 + map(constrained, -1000, 6000, 0, 270);
  
  for (int angle = 135; angle <= fillAngle; angle += 3) {
    float rad = angle * 3.14159 / 180;
    for (int r = radius - 8; r <= radius; r++) {
      int x = cx + cos(rad) * r;
      int y = cy + sin(rad) * r;
      epd->drawPixel(x, y, EPDColor::Black);
    }
  }
  
  // Needle
  float needleRad = fillAngle * 3.14159 / 180;
  for (int r = 10; r < radius - 12; r++) {
    int x = cx + cos(needleRad) * r;
    int y = cy + sin(needleRad) * r;
    epd->drawPixel(x, y, EPDColor::Black);
  }
  
  // Center
  for (int dx = -2; dx <= 2; dx++) {
    for (int dy = -2; dy <= 2; dy++) {
      epd->drawPixel(cx + dx, cy + dy, EPDColor::Black);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  cfg.width = 200;
  cfg.height = 200;
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
  
  epd->setTempCallback(onTempChange);
  epd->enableAutoTempLUT(true);
  epd->setRotation(0);
  
  Serial.println("Auto Temp LUT Demo");
}

void loop() {
  int16_t temp = epd->readTemperature();
  
  if (abs(temp - lastTemp) > 50) {
    lastTemp = temp;
    
    epd->fillScreen(EPDColor::White);
    
    // Temperature gauge
    drawGauge(100, 70, 50, temp);
    
    // Range indicator
    int8_t range = epd->getTemperatureRange(temp);
    const char* rangeLabels[] = {"<0", "0-10", "10-25", "25-40", "40-50", ">50"};
    
    for (int i = 0; i < 6; i++) {
      int bx = 20 + i * 30;
      int by = 140;
      
      // Box
      for (int x = 0; x < 25; x++) {
        epd->drawPixel(bx + x, by, EPDColor::Black);
        epd->drawPixel(bx + x, by + 25, EPDColor::Black);
      }
      for (int y = 0; y < 25; y++) {
        epd->drawPixel(bx, by + y, EPDColor::Black);
        epd->drawPixel(bx + 24, by + y, EPDColor::Black);
      }
      
      // Highlight current
      if (i == range) {
        for (int x = 1; x < 24; x++) {
          for (int y = 1; y < 24; y++) {
            epd->drawPixel(bx + x, by + y, EPDColor::Black);
          }
        }
      }
    }
    
    // Auto-LUT indicator
    if (epd->isAutoTempLUTEnabled()) {
      for (int i = 0; i < 15; i++) {
        epd->drawPixel(170 + i, 20 + i, EPDColor::Black);
        if (i < 10) epd->drawPixel(170 + i, 35 - i, EPDColor::Black);
      }
    }
    
    // VCOM Sense Duration
    epd->setVCOMSenseDuration(5);
    
    Serial.print("Temp: ");
    Serial.print(temp / 100.0);
    Serial.print(" C, Range: ");
    Serial.println(rangeLabels[range]);
    
    epd->flushFull();
  }
  
  delay(5000);
}
