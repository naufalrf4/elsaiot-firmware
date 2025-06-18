#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "WifiConf.h"
#include "OffsetSensor.h"

#define TFT_CS   15
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI 13
#define TFT_SCLK 18

#define TFT_W     320
#define TFT_H     240

#define C_BG      0x0000          // pure black
#define C_HDR1    0x001F          // blue gradient start
#define C_HDR2    0x051F          // blue gradient end
#define C_CARD_BG 0x18E3          // dark slate
#define C_LABEL   0xC618          // light grey
#define C_WARN    0xF922          // orange
#define C_OK      0x07E0          // green
#define C_VALUE   0xFFFF          // white

static const uint16_t HDR_H   = 36;
static const uint16_t CARD_W  = TFT_W - 24;
static const uint16_t CARD_H  = 42;
static const uint16_t CARD_X  = 12;
static const uint16_t CARD1_Y = 64;
static const uint16_t GAP     = 10;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

static inline void _hdrGradient() {
  for (uint16_t y = 0; y < HDR_H; ++y) {
    uint16_t c = tft.color565(
        map(y, 0, HDR_H, 0, 0),           // R
        map(y, 0, HDR_H, 0, 64),          // G
        map(y, 0, HDR_H, 255, 32));       // B
    tft.drawFastHLine(0, y, TFT_W, c);
  }
}

static inline void _drawCardBG(uint16_t y) {
  tft.fillRoundRect(CARD_X, y, CARD_W, CARD_H, 6, C_CARD_BG);
  tft.drawRoundRect(CARD_X, y, CARD_W, CARD_H, 6, 0x39E7);
}

static inline uint16_t _wifiColor() { return wifiIsOK() ? C_OK : C_WARN; }

static inline void initTFT() {
  SPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.begin();
  tft.setRotation(1);

  tft.fillScreen(C_BG);
  _hdrGradient();
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(C_VALUE, C_HDR1);
  tft.setCursor(12, 26);
  tft.print("ELSA IoT");

  uint16_t y = CARD1_Y;
  for (uint8_t i = 0; i < 4; ++i) {
    _drawCardBG(y);
    y += CARD_H + GAP;
  }
}

static inline void updateHeader() {
  static uint32_t last = 0;
  if (millis() - last < 5000) return;
  last = millis();

  _hdrGradient();
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(C_VALUE, C_HDR1);
  tft.setCursor(12, 26);
  if (wifiIsOK()) {
    tft.print("WiFi ONLINE");
  } else {
    tft.setTextColor(C_WARN, C_HDR1);
    tft.print("WiFi OFFLINE");
  }
}

static inline void _drawCard(uint16_t y,
                             const char *label,
                             const String &val,
                             uint16_t valColor) {
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(C_LABEL, C_CARD_BG);
  tft.setCursor(CARD_X + 10, y + 25);
  tft.print(label);

  int16_t bx, by; uint16_t bw, bh;
  tft.getTextBounds(val, 0, 0, &bx, &by, &bw, &bh);
  tft.setTextColor(valColor, C_CARD_BG);
  tft.setCursor(CARD_X + CARD_W - bw - 10, y + 25);
  tft.print(val);
}
\
uint16_t getPHColor(float v);
uint16_t getTDSColor(float v);
uint16_t getDOColor(float v);
uint16_t getTempColor(float v); 

static inline void drawTemp(float v) {
  _drawCard(CARD1_Y, "TEMP", String(v, 1) + " C", getTempColor(v));
}
static inline void drawPH(float v) {
  _drawCard(CARD1_Y + CARD_H + GAP, "pH", String(v, 2), getPHColor(v));
}
static inline void drawTDS(float v) {
  _drawCard(CARD1_Y + 2*(CARD_H + GAP), "TDS", String((int)v) + " ppm", getTDSColor(v));
}
static inline void drawDO(float v) {
  _drawCard(CARD1_Y + 3*(CARD_H + GAP), "DO", String(v, 2) + " mg/L", getDOColor(v));
}

static inline void showStatus(const String &msg, uint16_t col = C_OK) {
  tft.fillScreen(C_BG);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(col);
  int16_t x, y; uint16_t w, h;
  tft.getTextBounds(msg, 0, 0, &x, &y, &w, &h);
  tft.setCursor((TFT_W - w) / 2, (TFT_H + h) / 2);
  tft.print(msg);
  delay(1200);
}

static inline void showError(const String &msg) {
  showStatus(msg, C_WARN);
}

#include <RTClib.h>
static inline void showClock(const DateTime &now) {
  char buf[20];
  sprintf(buf, "%02d:%02d:%02d  %02d/%02d/%04d",
          now.hour(), now.minute(), now.second(),
          now.day(),  now.month(),  now.year());
  tft.fillRect(0, CARD1_Y - 22, TFT_W, 20, C_BG);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(0x07FF, C_BG);
  tft.setCursor(12, CARD1_Y - 8);
  tft.print(buf);
}

#endif