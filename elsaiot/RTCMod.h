#ifndef RTC_H
#define RTC_H

#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <time.h>
#include "WifiConf.h"
#include "TFT.h"

#define SDA_PIN 21
#define SCL_PIN 20

#define NTP_SERVER "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"
#define GMT_OFFSET_SEC 25200
#define DAYLIGHT_OFFSET_SEC 0

RTC_DS3231 rtc;
bool ntpSynced = false;

void initRTC() {
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!rtc.begin()) {
    Serial.println("❌ RTC not detected! Check SDA/SCL.");
    showError("RTC not found");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC lost power. Setting to compile time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("✅ RTC initialized.");
  }
}

bool syncTimeFromNTP() {
  if (!wifiIsOK()) {
    Serial.println("⚠️ WiFi not connected, skipping NTP sync");
    return false;
  }

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER, NTP_SERVER_2);

  struct tm timeinfo;
  uint8_t retry = 0;
  const uint8_t retryLimit = 10;
  bool gotTime = false;

  while (retry < retryLimit) {
    if (getLocalTime(&timeinfo, 1000)) {
      gotTime = true;
      break;
    }
    Serial.print(".");
    delay(500);
    retry++;
  }
  Serial.println();

  if (!gotTime) {
    Serial.println("❌ Failed to obtain time from NTP");
    return false;
  }

  DateTime ntpTime(
    timeinfo.tm_year + 1900,
    timeinfo.tm_mon + 1,
    timeinfo.tm_mday,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );
  rtc.adjust(ntpTime);

  Serial.printf("✅ RTC synchronized with NTP: %04d-%02d-%02d %02d:%02d:%02d\n",
                ntpTime.year(), ntpTime.month(), ntpTime.day(),
                ntpTime.hour(), ntpTime.minute(), ntpTime.second());

  ntpSynced = true;
  return true;
}

DateTime readRTC() {
  DateTime now;
  uint32_t startTime = millis();
  bool success = false;

  while (millis() - startTime < 100) {
    now = rtc.now();
    if (now.year() > 2000) {
      success = true;
      break;
    }
    delay(10);
    yield();
  }

  if (!success) {
    Serial.println("⚠️ RTC read timeout - using fallback time");
    return DateTime(F(__DATE__), F(__TIME__));
  }

  return now;
}

String formatTimestamp(DateTime now) {
  char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buf);
}

bool shouldSyncRTC() {
  static uint32_t lastSync = 0;
  uint32_t now = millis();

  if (!ntpSynced || (now - lastSync > 86400000)) {
    lastSync = now;
    return true;
  }

  return false;
}

float getRTCTemperature() {
  return rtc.getTemperature();
}

#endif