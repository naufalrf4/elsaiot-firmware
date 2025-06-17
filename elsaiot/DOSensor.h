#ifndef DO_SENSOR_H
#define DO_SENSOR_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>

#define DO_CHANNEL 2

extern Adafruit_ADS1115 ads;

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

extern float do_cal1_v, do_cal1_t;
extern float do_cal2_v, do_cal2_t; 
extern bool do_two_point_mode;
extern bool do_calibrated;

float last_do_calibrated = 0.0;

void initDOSensor() {
  Serial.println("🐟 DO Sensor initialized.");
}

float calculateDOFromVoltage(float voltage_mv, float temperature_c) {
  if (!do_calibrated) {
    return voltage_mv * 6.5 / 1000.0; // convert mg/L
  }
  
  uint8_t temp_index = constrain((int)temperature_c, 0, 40);
  uint16_t saturated_do = DO_Table[temp_index];
  
  float v_saturation;
  
  if (do_two_point_mode) {
    v_saturation = ((temperature_c - do_cal2_t) * (do_cal1_v - do_cal2_v) / (do_cal1_t - do_cal2_t)) + do_cal2_v;
  } else {
    v_saturation = do_cal1_v + 35.0 * temperature_c - do_cal1_t * 35.0;
  }
  
  if (v_saturation <= 0) {
    return 0.0;
  }
  
  float do_value = (voltage_mv * saturated_do / v_saturation) / 1000.0; // Convert μg/L to mg/L
  
  return constrain(do_value, 0.0, 20.0);
}

void readDOSensor(float* raw, float* voltage, float* calibrated, float temperature) {
  int16_t adc_raw = ads.readADC_SingleEnded(DO_CHANNEL);
  *voltage = adc_raw * (6.144 / 32767.0) * 1000.0;
  
  *raw = *voltage * 6.5 / 1000.0;
  
  *calibrated = calculateDOFromVoltage(*voltage, temperature);
  
  last_do_calibrated = *calibrated;
  
  if (*calibrated < 0) *calibrated = 0;
  if (*calibrated > 20) *calibrated = 20;
}

float getLastDOCalibrated() {
  return last_do_calibrated;
}

void applySinglePointDOCalibration(float voltage_mv, float temperature_c) {
  do_cal1_v = voltage_mv;
  do_cal1_t = temperature_c;
  do_two_point_mode = false;
  do_calibrated = true;
  
  Serial.printf("✅ DO Single-point calibration applied: %.1f mV at %.1f°C\n", voltage_mv, temperature_c);
}

void applyTwoPointDOCalibration(float voltage1_mv, float temp1_c, float voltage2_mv, float temp2_c) {
  if (temp1_c > temp2_c) {
    do_cal1_v = voltage1_mv;
    do_cal1_t = temp1_c;
    do_cal2_v = voltage2_mv;
    do_cal2_t = temp2_c;
  } else {
    do_cal1_v = voltage2_mv;
    do_cal1_t = temp2_c;
    do_cal2_v = voltage1_mv;
    do_cal2_t = temp1_c;
  }
  
  do_two_point_mode = true;
  do_calibrated = true;
  
  Serial.printf("✅ DO Two-point calibration applied: %.1f mV@%.1f°C, %.1f mV@%.1f°C\n", 
                do_cal1_v, do_cal1_t, do_cal2_v, do_cal2_t);
}

String getDOCalibrationStatus() {
  if (!do_calibrated) {
    return "uncalibrated";
  } else if (do_two_point_mode) {
    return "two-point";
  } else {
    return "single-point";
  }
}

#endif