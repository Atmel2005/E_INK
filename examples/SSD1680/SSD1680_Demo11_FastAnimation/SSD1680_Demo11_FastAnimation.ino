/**
 * SSD1680 Demo 11: Fast Animation
 * Flying shapes with quick partial refresh
 * Full refresh only once per minute
 * Resolution: 176x296
 */

#include <SPI.h>
#include <EPD_Base.h>
#include <EPD_SSD1680.h>

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

// Flying shapes
struct Shape {
  float x, y;
  float vx, vy;
  int size;
  int type;  // 0=circle, 1=square, 2=triangle
};

Shape shapes[5];
Shape prevShapes[5];  // Previous positions for clearing
unsigned long lastFullRefresh = 0;
unsigned long lastFrameUpdate = 0;
const unsigned long FULL_REFRESH_INTERVAL = 60000; // 1 minute
const unsigned long FRAME_INTERVAL = 1000; // Update display every 1000ms

void initShapes() {
  for (int i = 0; i < 5; i++) {
    shapes[i].x = random(30, 145);
    shapes[i].y = random(30, 265);
    shapes[i].vx = (random(0, 20) - 10) / 5.0;
    shapes[i].vy = (random(0, 20) - 10) / 5.0;
    shapes[i].size = random(10, 25);
    shapes[i].type = i % 3;
    prevShapes[i] = shapes[i];  // Initialize previous positions
  }
}

void drawShape(Shape& s, EPDColor color) {
  int x = (int)s.x;
  int y = (int)s.y;
  int r = s.size;
  
  switch (s.type) {
    case 0: // Circle - thin outline
      for (int angle = 0; angle < 360; angle += 5) {
        float rad = angle * 3.14159 / 180;
        int px = x + cos(rad) * r;
        int py = y + sin(rad) * r;
        epd->drawPixel(px, py, color);
      }
      break;
    case 1: // Square - thin outline
      for (int i = -r; i <= r; i++) {
        epd->drawPixel(x + i, y - r, color);
        epd->drawPixel(x + i, y + r, color);
        epd->drawPixel(x - r, y + i, color);
        epd->drawPixel(x + r, y + i, color);
      }
      break;
    case 2: // Triangle - thin outline
      for (int i = 0; i <= r; i++) {
        epd->drawPixel(x - i, y + r - i, color);
        epd->drawPixel(x + i, y + r - i, color);
      }
      for (int i = -r; i <= r; i++) {
        epd->drawPixel(x + i, y + r, color);
      }
      break;
  }
}

void updateShape(Shape& s) {
  s.x += s.vx;
  s.y += s.vy;
  
  // Bounce off walls (176x296 display)
  if (s.x < s.size + 5 || s.x > 171 - s.size) {
    s.vx = -s.vx;
    s.x = constrain(s.x, s.size + 5, 171 - s.size);
  }
  if (s.y < s.size + 5 || s.y > 291 - s.size) {
    s.vy = -s.vy;
    s.y = constrain(s.y, s.size + 5, 291 - s.size);
  }
}

void setup() {
  Serial.begin(115200);
  
  cfg.width = 176;
  cfg.height = 296;
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
  epd->setDisplayMode(DisplayMode::Mode2);
  epd->setRefreshProfile(RefreshProfile::Partial);
  
  initShapes();
  lastFullRefresh = millis();
  
  // Initial clear
  epd->fillScreen(EPDColor::White);
  epd->flushFull();
}

// Compute bounding box covering old and new shape positions
void computeBoundingBox(int& minX, int& minY, int& maxX, int& maxY) {
  minX = 176; minY = 296; maxX = 0; maxY = 0;
  for (int i = 0; i < 5; i++) {
    int r = shapes[i].size + 2; // padding
    // Old position
    int ox = prevShapes[i].x, oy = prevShapes[i].y;
    // New position
    int nx = shapes[i].x, ny = shapes[i].y;
    // Expand bbox
    minX = min(minX, min(ox - r, nx - r));
    minY = min(minY, min(oy - r, ny - r));
    maxX = max(maxX, max(ox + r, nx + r));
    maxY = max(maxY, max(oy + r, ny + r));
  }
  // Clamp to screen (176x296)
  minX = max(0, minX); minY = max(0, minY);
  maxX = min(175, maxX); maxY = min(295, maxY);
}

void loop() {
  unsigned long now = millis();
  
  // Full refresh every minute
  if (now - lastFullRefresh >= FULL_REFRESH_INTERVAL) {
    epd->fillScreen(EPDColor::White);
    epd->flushFull();
    lastFullRefresh = now;
    initShapes();
    lastFrameUpdate = now;
    return;
  }
  
  // Only update display every FRAME_INTERVAL
  if (now - lastFrameUpdate >= FRAME_INTERVAL) {
    // Erase old positions (white)
    for (int i = 0; i < 5; i++) {
      drawShape(prevShapes[i], EPDColor::White);
    }
    
    // Update positions
    for (int i = 0; i < 5; i++) {
      updateShape(shapes[i]);
    }
    
    // Draw new positions (black)
    for (int i = 0; i < 5; i++) {
      drawShape(shapes[i], EPDColor::Black);
    }
    
    // Compute bounding box of all changes
    int minX, minY, maxX, maxY;
    computeBoundingBox(minX, minY, maxX, maxY);
    int w = maxX - minX + 1;
    int h = maxY - minY + 1;
    
    // Update only the changed region using working flushRect
    epd->flushRect(minX, minY, w, h);
    
    // Save current positions as previous
    for (int i = 0; i < 5; i++) {
      prevShapes[i] = shapes[i];
    }
    
    lastFrameUpdate = now;
  }
}
