// HelloWorld ST7789 2.0" (SPI) – ESP32-S3
// Library: Adafruit GFX + Adafruit ST7735 and ST7789
// Pin mapping:
// SCLK=GPIO21, MOSI=GPIO47, CS=41, DC=40, RST=45, BLK=42

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ---- PIN KONFIG ----
#define PIN_TFT_SCLK 21    // SCL
#define PIN_TFT_MOSI 47    // SDA / MOSI
#define PIN_TFT_MISO -1    // tidak dipakai
#define PIN_TFT_CS   41
#define PIN_TFT_DC   40
#define PIN_TFT_RST  45
#define PIN_TFT_BL   42    // backlight (HIGH = ON)

// Resolusi ST7789 2.0"
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

// Konstruktor ST7789 (hardware SPI 40MHz)
SPIClass spi(FSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

void backlightOn() {
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
}

void setup() {
  backlightOn();

  // Hardware SPI 40MHz
  spi.begin(PIN_TFT_SCLK, PIN_TFT_MISO, PIN_TFT_MOSI);

  // Inisialisasi panel ST7789 240x320
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setSPISpeed(40000000);
  tft.setRotation(0);

  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, ST77XX_BLUE);

  tft.setTextWrap(false);

  tft.setCursor(15, 120);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.print("Hello");

  tft.setCursor(15, 168);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.print("ESP32-S3!");

  for (int i = 0; i < 10; i++) {
    tft.fillCircle(15 + i * 22, 280, 7, (i % 2) ? ST77XX_RED : ST77XX_CYAN);
  }
}

void loop() {
  static uint16_t c = ST77XX_RED;
  static bool toggle = false;
  tft.fillCircle(221, 20, 11, c);
  c = toggle ? ST77XX_RED : ST77XX_WHITE;
  toggle = !toggle;
  delay(400);
}
