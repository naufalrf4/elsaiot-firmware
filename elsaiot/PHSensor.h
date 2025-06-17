#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "Storage.h"

#define PH_CHANNEL 0 

extern Adafruit_ADS1115 ads;
extern float ph_regression_m;
extern float ph_regression_c;
extern bool calibrationReady;

void initPHSensor() {
  Serial.println("🌊 PH Sensor initialized.");
}

void readPHSensor(float* raw, float* voltage, float* calibrated) {
  uint32_t startTime = millis();
  int16_t adc_raw = 0;
  
  while (millis() - startTime < 100) {
    adc_raw = ads.readADC_SingleEnded(PH_CHANNEL);
    if (adc_raw != 0) break;
    yield();
    delay(1);
  }
  
  *voltage = adc_raw * (6.144 / 32767.0);
  
  *raw = *voltage * 3.5; 
  
  if (calibrationReady && !isnan(ph_regression_m) && !isnan(ph_regression_c)) {
    *calibrated = (ph_regression_m * (*voltage)) + ph_regression_c;
  } else {
    *calibrated = *raw;
  }
  
  if (*raw < 0) *raw = 0;
  if (*raw > 14) *raw = 14;
  if (*calibrated < 0) *calibrated = 0;
  if (*calibrated > 14) *calibrated = 14;
}

String getPHSensorStatus() {
  return calibrationReady ? "CALIBRATED" : "RAW";
}

bool isPHReadingValid(float voltage) {
  return (voltage >= 0.0 && voltage <= 6.144);
}

#endif