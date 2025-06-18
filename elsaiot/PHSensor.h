#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "Storage.h"

#define PH_CHANNEL 0

extern Adafruit_ADS1115 ads;

extern float ph_regression_m, ph_regression_c;
extern bool  ph_calibrated;

static inline void initPHSensor() { Serial.println("🌊 pH sensor ready"); }

static void readPHSensor(float *raw_pH, float *volt_V, float *cal_pH) {

    uint32_t t0 = millis();
    int16_t adc = 0;
    while (millis() - t0 < 100) {
        adc = ads.readADC_SingleEnded(PH_CHANNEL);
        if (adc != 0) break;
        yield();
        delay(1);
    }

    *volt_V = adc * (6.144f / 32767.0f);

    *raw_pH = (*volt_V) * 3.5f;

    if (ph_calibrated && !isnan(ph_regression_m) && !isnan(ph_regression_c)) {
        *cal_pH = (ph_regression_m * (*volt_V)) + ph_regression_c;
    } else {
        *cal_pH = *raw_pH;
    }

    *raw_pH = constrain(*raw_pH, 0.0f, 14.0f);
    *cal_pH = constrain(*cal_pH, 0.0f, 14.0f);
}

static inline String getPHSensorStatus() {
    return ph_calibrated ? F("CALIBRATED") : F("RAW");
}

static inline bool isPHReadingValid(float voltV) {
    return (voltV >= 0.0f && voltV <= 6.144f);
}

#endif