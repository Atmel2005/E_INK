/**
 * SSD1681 Demo 07: RAM Readback
 * Read and verify display RAM content
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
  
  epd->setRotation(0);
}

void loop() {
  // Draw test pattern
  epd->fillScreen(EPDColor::White);
  
  // Checkerboard
  for (int y = 10; y < 90; y++) {
    for (int x = 10; x < 90; x++) {
      if ((x/10 + y/10) % 2 == 0) {
        epd->drawPixel(x, y, EPDColor::Black);
      }
    }
  }
  
  // Stripes
  for (int y = 110; y < 190; y++) {
    for (int x = 10; x < 190; x++) {
      if (x % 20 < 10) {
        epd->drawPixel(x, y, EPDColor::Black);
      }
    }
  }
  
  // Circle
  for (int angle = 0; angle < 360; angle += 2) {
    float rad = angle * 3.14159 / 180;
    int x = 150 + cos(rad) * 30;
    int y = 50 + sin(rad) * 30;
    epd->drawPixel(x, y, EPDColor::Black);
  }
  
  epd->flushFull();
  delay(2000);
  
  // Read back RAM
  Serial.println("=== RAM Readback Test ===");
  
  uint8_t buffer[32];
  epd->readRAM(0, 0, buffer, 32);
  
  Serial.println("First 32 bytes:");
  for (int i = 0; i < 32; i++) {
    if (i % 8 == 0) Serial.print("  ");
    Serial.print("0x");
    if (buffer[i] < 16) Serial.print("0");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
    if ((i + 1) % 8 == 0) Serial.println();
  }
  
  // Compare with framebuffer
  uint8_t* fb = epd->framebuffer();
  if (fb) {
    int matches = 0;
    for (int i = 0; i < 32; i++) {
      if (buffer[i] == fb[i]) matches++;
    }
    Serial.print("Match: ");
    Serial.print(matches);
    Serial.println("/32 bytes");
  }
  
  // Read Chip ID
  uint8_t id = epd->readChipID();
  Serial.print("Chip ID: 0x");
  Serial.println(id, HEX);
  
  // Display verification
  epd->fillScreen(EPDColor::White);
  
  // Verification icon
  int cx = 100, cy = 80;
  for (int angle = 0; angle < 360; angle += 5) {
    float rad = angle * 3.14159 / 180;
    int x = cx + cos(rad) * 40;
    int y = cy + sin(rad) * 40;
    epd->drawPixel(x, y, EPDColor::Black);
  }
  
  // Checkmark
  for (int i = 0; i < 25; i++) {
    epd->drawPixel(cx - 12 + i, cy + i - 8, EPDColor::Black);
    if (i < 18) epd->drawPixel(cx - 12 + i, cy - 8 - i + 18, EPDColor::Black);
  }
  
  // Chip ID display
  for (int i = 0; i < 60; i++) {
    epd->drawPixel(70 + i, 150, EPDColor::Black);
  }
  
  epd->flushFull();
  delay(10000);
}
