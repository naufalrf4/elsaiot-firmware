#ifndef INIT_ADS_H
#define INIT_ADS_H

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

#define SDA_PIN 21
#define SCL_PIN 20

Adafruit_ADS1115 ads;

inline void initADS() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  Wire.setClock(100000);
  
  if (!ads.begin()) {
    Serial.println("❌ ADS1115 not detected! Check wiring.");
    while (1) {
      delay(1000);
      yield();
    }
  }
  
  ads.setGain(GAIN_TWOTHIRDS);
  
  ads.setDataRate(RATE_ADS1115_128SPS);
  
  Serial.println("📈 ADS1115 initialized.");
  
  for (int channel = 0; channel < 4; channel++) {
    int16_t test_read = ads.readADC_SingleEnded(channel);
    Serial.printf("   - Channel %d: %d (%.3fV)\n", 
                  channel, test_read, test_read * (6.144 / 32767.0));
    delay(10);
  }
}

float readChannelVoltage(int channel) {
  if (channel < 0 || channel > 3) {
    Serial.println("❌ Invalid ADS channel");  
    return -1.0;
  }
  
  int16_t adc_value = ads.readADC_SingleEnded(channel);
  return adc_value * (6.144 / 32767.0);
}

bool isADSHealthy() {
  int16_t test_read = ads.readADC_SingleEnded(0);
  
  if (test_read == -1 || test_read == 0x8000) {
    Serial.println("⚠️  ADS1115 may not be responding properly");
    return false;
  }
  
  return true;
}

void resetADS() {
  Serial.println("🔄 Resetting ADS1115...");
  
  Wire.end();
  delay(100);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  
  ads.begin();
  ads.setGain(GAIN_TWOTHIRDS);
  ads.setDataRate(RATE_ADS1115_128SPS);
  
  Serial.println("✅ ADS1115 reset complete");
}

#endif