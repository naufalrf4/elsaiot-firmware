#ifndef TDS_SENSOR_H
#define TDS_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>

#define TDS_CHANNEL 1

extern Adafruit_ADS1115 ads;

extern float tds_k_value;
extern bool tds_calibrated;

static float lastTDSmgL = 0.0;

static void initTDSSensor() {
    Serial.println("🌊 TDS Sensor initialized.");
}

static float calculateTDSFromVoltage(float voltage, float temperature) {
    if (!tds_calibrated) {
        return voltage * 1000.0;
    }
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = voltage / compensationCoefficient;
    float tds_value = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage 
                    - 255.86 * compensationVoltage * compensationVoltage 
                    + 857.39 * compensationVoltage) * 0.5 * tds_k_value;
    return constrain(tds_value, 0.0, 1000.0);
}

static void readTDSSensor(float* raw, float* voltage, float* calibrated, float temperature) {
    int16_t adc_raw = ads.readADC_SingleEnded(TDS_CHANNEL);
    *voltage = adc_raw * (6.144 / 32767.0);

    *raw = *voltage * 1000.0;

    *calibrated = calculateTDSFromVoltage(*voltage, temperature);

    lastTDSmgL = *calibrated;
    if (*calibrated < 0) *calibrated = 0;
    if (*calibrated > 1000) *calibrated = 1000;
}

static float getLastTDSCalibrated() {
    return lastTDSmgL;
}

static void applyTDSCalibration(float voltage, float standard_value, float temperature) {
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = voltage / compensationCoefficient;
    float tds_value_raw = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage 
                        - 255.86 * compensationVoltage * compensationVoltage 
                        + 857.39 * compensationVoltage) * 0.5;
    tds_k_value = standard_value / tds_value_raw;
    tds_calibrated = true;
    Serial.printf("✅ TDS Calibration applied: K=%.3f (%.1f ppm at %.3f V, %.1f°C)\n", 
                tds_k_value, standard_value, voltage, temperature);
}

static String getTDSCalibrationStatus() {
    if (!tds_calibrated) {
        return F("uncalibrated");
    } else {
        return F("calibrated");
    }
}

#endif