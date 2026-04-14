## Deutsch

# E_INK Library - E-Paper Display Treiber

Einheitlicher Treiber für E-Paper-Displays (EPD) mit Unterstützung für mehrere Controller und SPI-Backends.

Repository: https://github.com/Atmel2005/E_INK

---

## Funktionen

- **Mehrere Controller**: SSD1680, SSD1681, SSD1608, UC8151, IL3829
- **Farbvarianten**: Schwarz/Weiß, Schwarz/Weiß/Rot, Schwarz/Weiß/Gelb, Schwarz/Weiß/Rot/Gelb
- **SPI-Modi**: Hardware-SPI, Software-SPI, 3-Draht und 4-Draht
- **Aktualisierungsmodi**: Vollständige Aktualisierung und Teilaktualisierung
- **Drehungsunterstützung**: 0°, 90°, 180°, 270°
- **Multi-Plattform**: RP2040, ESP32, STM32, AVR (Arduino Uno)

---

## Installation

1. Repository herunterladen oder klonen
2. Den Ordner `E_INK` in das Arduino-Bibliothekenverzeichnis kopieren:
   - Windows: `Dokumente\Arduino\libraries\`
   - macOS/Linux: `~/Arduino/libraries/`
3. Arduino IDE neu starten

---

## Schnellstart

```cpp
#include <E_INK.h>

EPDConfig cfg;
EPD_Base* epd = nullptr;

void setup() {
  cfg.width = 200;
  cfg.height = 200;
  cfg.controller = EPDController::SSD1681;
  cfg.variant = EPDVariant::BW;
  cfg.cs = 10;
  cfg.dc = 9;
  cfg.rst = 8;
  cfg.busy = 7;
  cfg.use_hw_spi = true;
  
  epd = createEPD(cfg);
  epd->begin();
  epd->fillScreen(EPDColor::White);
  epd->drawPixel(100, 100, EPDColor::Black);
  epd->flushFull();
  epd->sleep();
}

void loop() {}
```

---

## Unterstützte Controller

| Controller | Auflösung | Farben | Hinweise |
| ---------- | --------- | ------ | -------- |
| SSD1680 | 122x250 | BW, BW_R | Gängige 2.13" Displays |
| SSD1681 | 200x200 | BW, BW_R, BW_Y | 1.54" Displays |
| SSD1608 | 200x200 | BW | Ältere 1.54" Displays |
| UC8151 | Variabel | BW | Niederspannungs-Displays |
| IL3829 | Variabel | BW | GDEH0213B73 Displays |

---

## API-Referenz

| Funktion | Beschreibung |
| -------- | ------------ |
| `begin()` | Display initialisieren |
| `end()` | Display deinitialisieren |
| `sleep()` | Deep-Sleep-Modus |
| `drawPixel(x, y, color)` | Einzelnes Pixel zeichnen |
| `fillScreen(color)` | Bildschirm füllen |
| `flushFull()` | Vollständige Aktualisierung |
| `flushRect(x, y, w, h)` | Teilaktualisierung |
| `setRotation(r)` | Drehung setzen (0-3) |
| `framebuffer()` | Schwarz/Weiß-Puffer |
| `framebufferAccent()` | Farb-Puffer (Rot/Gelb) |

---

## Pin-Belegung

| Pin | RP2040 | ESP32 | Arduino Uno |
| --- | ------ | ----- | ----------- |
| MOSI | 7 | 23 | 11 |
| SCLK | 6 | 18 | 13 |
| CS | 10 | 5 | 10 |
| DC | 9 | 17 | 9 |
| RST | 8 | 16 | 8 |
| BUSY | 11 | 4 | 7 |

---

## Links

- Repository: https://github.com/Atmel2005/E_INK

---

## Українською

# E_INK Library - Драйвер e-paper дисплеїв

Універсальний драйвер для e-paper дисплеїв (EPD) з підтримкою багатьох контролерів та SPI-інтерфейсів.

Репозиторій: https://github.com/Atmel2005/E_INK

---

## Можливості

- **Багато контролерів**: SSD1680, SSD1681, SSD1608, UC8151, IL3829
- **Кольорові варіанти**: Чорно-білий, Чорно-біло-червоний, Чорно-біло-жовтий, Чорно-біло-червоно-жовтий
- **SPI-режими**: Апаратний SPI, Програмний SPI, 3-провідний та 4-провідний
- **Режими оновлення**: Повне та часткове оновлення
- **Підтримка повороту**: 0°, 90°, 180°, 270°
- **Мультиплатформовість**: RP2040, ESP32, STM32, AVR (Arduino Uno)

---

## Встановлення

1. Завантажити або клонувати репозиторій
2. Копіювати папку `E_INK` у директорію бібліотек Arduino:
   - Windows: `Документи\Arduino\libraries\`
   - macOS/Linux: `~/Arduino/libraries/`
3. Перезавантажити Arduino IDE

---

## Швидкий старт

```cpp
#include <E_INK.h>

EPDConfig cfg;
EPD_Base* epd = nullptr;

void setup() {
  cfg.width = 200;
  cfg.height = 200;
  cfg.controller = EPDController::SSD1681;
  cfg.variant = EPDVariant::BW;
  cfg.cs = 10;
  cfg.dc = 9;
  cfg.rst = 8;
  cfg.busy = 7;
  cfg.use_hw_spi = true;
  
  epd = createEPD(cfg);
  epd->begin();
  epd->fillScreen(EPDColor::White);
  epd->drawPixel(100, 100, EPDColor::Black);
  epd->flushFull();
  epd->sleep();
}

void loop() {}
```

---

## Підтримувані контролери

| Контролер | Роздільна здатність | Кольори | Примітка |
| ---------- | ------------------- | ------- | -------- |
| SSD1680 | 122x250 | BW, BW_R | Популярні 2.13" дисплеї |
| SSD1681 | 200x200 | BW, BW_R, BW_Y | 1.54" дисплеї |
| SSD1608 | 200x200 | BW | Старі 1.54" дисплеї |
| UC8151 | Змінна | BW | Низьковольтні дисплеї |
| IL3829 | Змінна | BW | GDEH0213B73 дисплеї |

---

## API-довідник

| Функція | Опис |
| ------- | ---- |
| `begin()` | Ініціалізувати дисплей |
| `end()` | Деініціалізувати дисплей |
| `sleep()` | Режим глибокого сну |
| `drawPixel(x, y, color)` | Малювати піксель |
| `fillScreen(color)` | Заповнити екран |
| `flushFull()` | Повне оновлення |
| `flushRect(x, y, w, h)` | Часткове оновлення |
| `setRotation(r)` | Встановити поворот (0-3) |
| `framebuffer()` | Буфер чорно-білого |
| `framebufferAccent()` | Кольоровий буфер |

---

## Підключення пінів

| Пін | RP2040 | ESP32 | Arduino Uno |
| --- | ------ | ----- | ----------- |
| MOSI | 7 | 23 | 11 |
| SCLK | 6 | 18 | 13 |
| CS | 10 | 5 | 10 |
| DC | 9 | 17 | 9 |
| RST | 8 | 16 | 8 |
| BUSY | 11 | 4 | 7 |

---

## Посилання

- Репозиторій: https://github.com/Atmel2005/E_INK

---

## English 

# E_INK Library - E-Paper Display Driver

Unified driver for e-paper displays (EPD) with support for multiple controllers and SPI backends.

Repository: https://github.com/Atmel2005/E_INK

---

## Features

- **Multiple controllers**: SSD1680, SSD1681, SSD1608, UC8151, IL3829
- **Color variants**: Black/White, Black/White/Red, Black/White/Yellow, Black/White/Red/Yellow
- **SPI modes**: Hardware SPI, Software SPI, 3-wire and 4-wire
- **Refresh modes**: Full refresh and partial update
- **Rotation support**: 0°, 90°, 180°, 270°
- **Multi-platform**: RP2040, ESP32, STM32, AVR (Arduino Uno)

---

## Installation

1. Download or clone this repository
2. Copy the `E_INK` folder to your Arduino libraries directory:
   - Windows: `Documents\Arduino\libraries\`
   - macOS/Linux: `~/Arduino/libraries/`
3. Restart Arduino IDE

---

## Quick Start

```cpp
#include <E_INK.h>

EPDConfig cfg;
EPD_Base* epd = nullptr;

void setup() {
  cfg.width = 200;
  cfg.height = 200;
  cfg.controller = EPDController::SSD1681;
  cfg.variant = EPDVariant::BW;
  cfg.cs = 10;
  cfg.dc = 9;
  cfg.rst = 8;
  cfg.busy = 7;
  cfg.use_hw_spi = true;
  
  epd = createEPD(cfg);
  epd->begin();
  epd->fillScreen(EPDColor::White);
  epd->drawPixel(100, 100, EPDColor::Black);
  epd->flushFull();
  epd->sleep();
}

void loop() {}
```

---

## Supported Controllers

| Controller | Resolution | Colors | Notes |
| ---------- | ---------- | ------ | ----- |
| SSD1680 | 122x250 | BW, BW_R | Common 2.13" displays |
| SSD1681 | 200x200 | BW, BW_R, BW_Y | 1.54" displays |
| SSD1608 | 200x200 | BW | Older 1.54" displays |
| UC8151 | Various | BW | Low-voltage displays |
| IL3829 | Various | BW | GDEH0213B73 displays |

---

## API Reference

| Function | Description |
| -------- | ----------- |
| `begin()` | Initialize display |
| `end()` | Deinitialize display |
| `sleep()` | Enter deep sleep mode |
| `drawPixel(x, y, color)` | Draw a single pixel |
| `fillScreen(color)` | Fill entire screen |
| `flushFull()` | Full display refresh |
| `flushRect(x, y, w, h)` | Partial refresh |
| `setRotation(r)` | Set rotation (0-3) |
| `framebuffer()` | Get black/white buffer |
| `framebufferAccent()` | Get color buffer (red/yellow) |

---

## Pin Connections

| Pin | RP2040 | ESP32 | Arduino Uno |
| --- | ------ | ----- | ----------- |
| MOSI | 7 | 23 | 11 |
| SCLK | 6 | 18 | 13 |
| CS | 10 | 5 | 10 |
| DC | 9 | 17 | 9 |
| RST | 8 | 16 | 8 |
| BUSY | 11 | 4 | 7 |

---

## Links

- Repository: https://github.com/Atmel2005/E_INK

---

## Русский

# E_INK Library - Драйвер e-paper дисплеев

Универсальный драйвер для e-paper дисплеев (EPD) с поддержкой нескольких контроллеров и SPI-интерфейсов.

Репозиторий: https://github.com/Atmel2005/E_INK

---

## Возможности

- **Несколько контроллеров**: SSD1680, SSD1681, SSD1608, UC8151, IL3829
- **Цветовые варианты**: Черно-белый, Черно-бело-красный, Черно-бело-желтый, Черно-бело-красно-желтый
- **Режимы SPI**: Аппаратный SPI, Программный SPI, 3-проводной и 4-проводной
- **Режимы обновления**: Полное и частичное обновление
- **Поддержка поворота**: 0°, 90°, 180°, 270°
- **Мультиплатформенность**: RP2040, ESP32, STM32, AVR (Arduino Uno)

---

## Установка

1. Скачать или клонировать репозиторий
2. Скопировать папку `E_INK` в директорию библиотек Arduino:
   - Windows: `Документы\Arduino\libraries\`
   - macOS/Linux: `~/Arduino/libraries/`
3. Перезапустить Arduino IDE

---

## Быстрый старт

```cpp
#include <E_INK.h>

EPDConfig cfg;
EPD_Base* epd = nullptr;

void setup() {
  cfg.width = 200;
  cfg.height = 200;
  cfg.controller = EPDController::SSD1681;
  cfg.variant = EPDVariant::BW;
  cfg.cs = 10;
  cfg.dc = 9;
  cfg.rst = 8;
  cfg.busy = 7;
  cfg.use_hw_spi = true;
  
  epd = createEPD(cfg);
  epd->begin();
  epd->fillScreen(EPDColor::White);
  epd->drawPixel(100, 100, EPDColor::Black);
  epd->flushFull();
  epd->sleep();
}

void loop() {}
```

---

## Поддерживаемые контроллеры

| Контроллер | Разрешение | Цвета | Примечание |
| ---------- | ----------- | ----- | ---------- |
| SSD1680 | 122x250 | BW, BW_R | Популярные 2.13" дисплеи |
| SSD1681 | 200x200 | BW, BW_R, BW_Y | 1.54" дисплеи |
| SSD1608 | 200x200 | BW | Старые 1.54" дисплеи |
| UC8151 | Разное | BW | Низковольтные дисплеи |
| IL3829 | Разное | BW | GDEH0213B73 дисплеи |

---

## API-справочник

| Функция | Описание |
| ------- | -------- |
| `begin()` | Инициализировать дисплей |
| `end()` | Деинициализировать дисплей |
| `sleep()` | Режим глубокого сна |
| `drawPixel(x, y, color)` | Рисовать пиксель |
| `fillScreen(color)` | Заполнить экран |
| `flushFull()` | Полное обновление |
| `flushRect(x, y, w, h)` | Частичное обновление |
| `setRotation(r)` | Установить поворот (0-3) |
| `framebuffer()` | Буфер черно-белого |
| `framebufferAccent()` | Цветовой буфер |

---

## Схема подключения

| Контакт | RP2040 | ESP32 | Arduino Uno |
| ------- | ------ | ----- | ----------- |
| MOSI | 7 | 23 | 11 |
| SCLK | 6 | 18 | 13 |
| CS | 10 | 5 | 10 |
| DC | 9 | 17 | 9 |
| RST | 8 | 16 | 8 |
| BUSY | 11 | 4 | 7 |

---

## Ссылки

- Репозиторий: https://github.com/Atmel2005/E_INK

---

## License

MIT License

## Author

ATMEL (atmel2005)
