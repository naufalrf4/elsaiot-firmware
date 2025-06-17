#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "WifiConf.h"

#define TFT_CS    15
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  13 
#define TFT_SCLK  18
#define WIDTH   320
#define HEIGHT  240

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

#define HDR_H        34
#define CARD_X       12
#define CARD_W       (WIDTH - 24)
#define CARD_H       38
#define CARD_GAP     8
#define CARD1_Y      60
#define CARD2_Y      (CARD1_Y + CARD_H + CARD_GAP)
#define CARD3_Y      (CARD2_Y + CARD_H + CARD_GAP)
#define CARD4_Y      (CARD3_Y + CARD_H + CARD_GAP)

#define C_BG     0x0000
#define C_HDR    0x000F
#define C_CARD   0x2104
#define C_LABEL  0xC618
#define C_WARN   0xFD20

// Forward declarations
uint16_t getPHColor(float value);
uint16_t getTDSColor(float value);
uint16_t getDOColor(float value);

static void _splash() {
  tft.fillScreen(C_BG);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(0xFFE0);
  const char* msg = "E L S A  I o T";
  int16_t x, y; uint16_t w, h;
  tft.getTextBounds(msg, 0, 0, &x, &y, &w, &h);
  tft.setCursor((WIDTH - w) / 2, (HEIGHT + h) / 2);
  tft.print(msg);
  delay(5000);
}

inline void initTFT(uint8_t brightness = 255) {
  SPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.begin();
  tft.setRotation(1);
  _splash();
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, WIDTH, HDR_H, C_HDR);
  for (uint8_t i = 0; i < 4; i++) {
    uint16_t y = CARD1_Y + i * (CARD_H + CARD_GAP);
    tft.fillRoundRect(CARD_X, y, CARD_W, CARD_H, 6, C_CARD);
  }
  tft.setFont(&FreeSansBold12pt7b);
}

inline void updateHeader() {
  static bool toggle = false;
  static uint32_t last = 0;
  if (millis() - last < 5000) return;
  last = millis();
  toggle = !toggle;

  tft.fillRect(0, 0, WIDTH, HDR_H, C_HDR);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(0xFFFF, C_HDR);
  tft.setCursor(12, 24);

  if (toggle) {
    tft.print("ELSA IoT");
  } else {
    if (wifiIsOK()) {
      tft.print("WiFi ONLINE");
    } else {
      tft.setTextColor(C_WARN, C_HDR);
      tft.print("WiFi OFFLINE");
    }
  }
}

static void _drawCard(uint16_t y, const char* label, const String& val, uint16_t color) {
  tft.setFont();
  tft.setTextColor(C_LABEL, C_CARD);
  tft.setCursor(CARD_X + 8, y + 24);
  tft.print(label);

  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(val, 0, 0, &x1, &y1, &w, &h);
  tft.setTextColor(color, C_CARD);
  tft.setCursor(CARD_X + CARD_W - w - 8, y + 24);
  tft.print(val);
}

inline void drawTemp(float v) { 
  _drawCard(CARD1_Y, "Temp", String(v, 1) + " C", 0xFFE0); 
}

inline void drawPH(float v) { 
  _drawCard(CARD2_Y, "pH", String(v, 2), getPHColor(v)); 
}

inline void drawTDS(float v) { 
  _drawCard(CARD3_Y, "TDS", String(v, 0) + " ppm", getTDSColor(v)); 
}

inline void drawDO(float v) { 
  _drawCard(CARD4_Y, "DO", String(v, 2) + " mg/L", getDOColor(v)); 
}

inline void showStatus(const String &message) {
  tft.fillScreen(C_BG);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(0xFFFF);
  int16_t x, y; uint16_t w, h;
  tft.getTextBounds(message, 0, 0, &x, &y, &w, &h);
  tft.setCursor((WIDTH - w) / 2, HEIGHT / 2);
  tft.print(message);
  delay(1500);
}

inline void showError(const String &message) {
  tft.fillScreen(C_BG);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(0xF800);
  int16_t x, y; uint16_t w, h;
  tft.getTextBounds(message, 0, 0, &x, &y, &w, &h);
  tft.setCursor((WIDTH - w) / 2, HEIGHT / 2);
  tft.print(message);
  delay(3000);
}

inline void showClock(const DateTime& now) {
  char timeStr[16], dateStr[20];
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  sprintf(dateStr, "%02d/%02d/%04d", now.day(), now.month(), now.year());

  tft.fillRect(12, 40, WIDTH - 24, 20, C_BG);
  tft.setFont();
  tft.setTextColor(0x07FF, C_BG);
  tft.setCursor(CARD_X + 8, 52);
  tft.print(timeStr);

  tft.setTextColor(C_LABEL, C_BG);
  tft.setCursor(WIDTH - 120, 52);
  tft.print(dateStr);
}

#endif
