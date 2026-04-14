/**
 * SSD1680 Demo 07: RAM Readback
 * Demonstrates reading back display RAM content
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
  
  epd->setRotation(1);
}

void loop() {
  // Draw a test pattern
  epd->fillScreen(EPDColor::White);
  
  // Draw checkerboard pattern
  for (int y = 0; y < 50; y++) {
    for (int x = 0; x < 100; x++) {
      if ((x/10 + y/10) % 2 == 0) {
        epd->drawPixel(10 + x, 10 + y, EPDColor::Black);
      }
    }
  }
  
  // Draw some lines
  for (int i = 0; i < 50; i++) {
    epd->drawPixel(10 + i, 70 + i, EPDColor::Black);
    epd->drawPixel(60 + i, 70 + i, EPDColor::Black);
  }
  
  epd->flushFull();
  delay(2000);
  
  // Now read back RAM content
  Serial.println("Reading RAM content...");
  
  uint8_t buffer[16];
  epd->readRAM(0, 0, buffer, 16);
  
  Serial.println("First 16 bytes of RAM:");
  for (int i = 0; i < 16; i++) {
    Serial.print("0x");
    if (buffer[i] < 16) Serial.print("0");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
    if ((i + 1) % 8 == 0) Serial.println();
  }
  
  // Verify by comparing with framebuffer
  uint8_t* fb = epd->framebuffer();
  if (fb) {
    Serial.println("\nFramebuffer comparison:");
    bool match = true;
    for (int i = 0; i < 16; i++) {
      if (buffer[i] != fb[i]) {
        match = false;
        Serial.print("Mismatch at ");
        Serial.print(i);
        Serial.print(": RAM=0x");
        Serial.print(buffer[i], HEX);
        Serial.print(" FB=0x");
        Serial.println(fb[i], HEX);
      }
    }
    if (match) Serial.println("RAM matches framebuffer!");
  }
  
  // Read Chip ID
  uint8_t chipId = epd->readChipID();
  Serial.print("Chip ID: 0x");
  Serial.println(chipId, HEX);
  
  // Display verification result
  epd->fillScreen(EPDColor::White);
  
  // Draw verification icon
  int16_t cx = epd->width() / 2;
  int16_t cy = 40;
  
  // Circle
  for (int angle = 0; angle < 360; angle += 5) {
    float rad = angle * 3.14159 / 180;
    int x = cx + cos(rad) * 25;
    int y = cy + sin(rad) * 25;
    epd->drawPixel(x, y, EPDColor::Black);
  }
  
  // Checkmark
  for (int i = 0; i < 15; i++) {
    epd->drawPixel(cx - 8 + i, cy + i - 3, EPDColor::Black);
    if (i < 10) epd->drawPixel(cx - 8 + i, cy - 3 - i + 10, EPDColor::Black);
  }
  
  // Chip ID display
  for (int i = 0; i < 40; i++) {
    epd->drawPixel(40 + i, 80, EPDColor::Black);
  }
  
  epd->flushFull();
  delay(10000);
}
