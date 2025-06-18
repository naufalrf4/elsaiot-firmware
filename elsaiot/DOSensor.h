#ifndef DO_SENSOR_H
#define DO_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "GlobalVars.h"

#define DO_CHANNEL 2

extern Adafruit_ADS1115 ads;

static const uint16_t DO_Table[41] = {
    14460,14220,13820,13440,13090,12740,12420,12110,11810,11530,
    11260,11010,10770,10530,10300,10080, 9860, 9660, 9460, 9270,
     9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
     7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

static float lastDOmgL = 0.0;

static inline void initDOSensor() {
    Serial.println("🐟 DO sensor ready");
}

static float calcDOmgL(float v_mV, float tempC) {
    if (!do_calibrated)
        return (v_mV * 6.5f) / 1000.0f;

    uint8_t idx = constrain((int)tempC, 0, 40);
    uint16_t sat = DO_Table[idx];

    float vSat;
    if (do_two_point_mode) {
        vSat = ((tempC - do_cal2_t) * (do_cal1_v - do_cal2_v) /
                (do_cal1_t - do_cal2_t)) + do_cal2_v;
    } else {
        vSat = do_cal1_v + 35.0f * (tempC - do_cal1_t);
    }

    if (vSat <= 0.0f) return 0.0f;

    float mgL = (v_mV * sat) / (vSat * 1000.0f);
    return constrain(mgL, 0.0f, 20.0f);
}

static void readDOSensor(float *raw_mgL, float *volt_mV, float *cal_mgL, float tempC) {
    int16_t adc = ads.readADC_SingleEnded(DO_CHANNEL);

    *volt_mV = adc * (6.144f / 32767.0f) * 1000.0f;

    *raw_mgL = (*volt_mV * 6.5f) / 1000.0f;
    *cal_mgL = calcDOmgL(*volt_mV, tempC);

    lastDOmgL = *cal_mgL;
}

static inline float getLastDO() { return lastDOmgL; }

static void applySinglePointDOCalibration(float v_mV, float tempC) {
    do_cal1_v         = v_mV;
    do_cal1_t         = tempC;
    do_two_point_mode = false;
    do_calibrated     = true;
    Serial.printf("✅ DO single-point: %.1f mV @ %.1f °C\n", v_mV, tempC);
}

static void applyTwoPointDOCalibration(float v1_mV, float t1_C, float v2_mV, float t2_C) {
    if (t1_C > t2_C) {
        do_cal1_v = v1_mV; do_cal1_t = t1_C;
        do_cal2_v = v2_mV; do_cal2_t = t2_C;
    } else {
        do_cal1_v = v2_mV; do_cal1_t = t2_C;
        do_cal2_v = v1_mV; do_cal2_t = t1_C;
    }
    do_two_point_mode = true;
    do_calibrated     = true;
    Serial.printf("✅ DO two-point: %.1f mV@%.1f °C  %.1f mV@%.1f °C\n",
                  do_cal1_v, do_cal1_t, do_cal2_v, do_cal2_t);
}

static String getDOCalibrationStatus() {
    if (!do_calibrated)           return F("uncalibrated");
    return do_two_point_mode ? F("two-point") : F("single-point");
}

#endif