#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include <Arduino.h>
#include "GlobalVars.h"

Preferences prefs;

extern float ph_regression_m;
extern float ph_regression_c;
extern bool ph_calibrated;

extern float tds_regression_m;
extern float tds_regression_c;
extern bool tds_calibrated;
extern float tds_k_value;

extern float do_regression_m;
extern float do_regression_c;
extern bool do_calibrated;
extern float do_cal1_v;
extern float do_cal1_t;
extern float do_cal2_v;
extern float do_cal2_t;
extern bool do_two_point_mode;

extern float ph_good;
extern float ph_bad;
extern int tds_good;
extern int tds_bad;
extern float do_good;
extern float do_bad;

void initStorage() {
  Serial.println("📁 Initializing NVS storage...");
  
  ph_regression_m = 1.0;
  ph_regression_c = 0.0;
  ph_calibrated = false;
  
  tds_regression_m = 1.0;
  tds_regression_c = 0.0;
  tds_calibrated = false;
  tds_k_value = 1.0;
  
  do_regression_m = 1.0;
  do_regression_c = 0.0;
  do_calibrated = false;
  do_cal1_v = 1.6;
  do_cal1_t = 25.0;
  do_cal2_v = 1.0;
  do_cal2_t = 25.0;
  do_two_point_mode = false;
  
  ph_good = 6.5;
  ph_bad = 8.0;
  tds_good = 400;
  tds_bad = 800;
  do_good = 7.0;
  do_bad = 4.0;
  
  Serial.println("✅ Storage initialized with default values");
}

void loadCalibrationData() {
  prefs.begin("elsa_cal", true);
  
  ph_regression_m = prefs.getFloat("ph_m", 1.0);
  ph_regression_c = prefs.getFloat("ph_c", 0.0);
  ph_calibrated = prefs.getBool("ph_cal", false);
  
  tds_regression_m = prefs.getFloat("tds_m", 1.0);
  tds_regression_c = prefs.getFloat("tds_c", 0.0);
  tds_calibrated = prefs.getBool("tds_cal", false);
  tds_k_value = prefs.getFloat("tds_k", 1.0);
  
  do_regression_m = prefs.getFloat("do_m", 1.0);
  do_regression_c = prefs.getFloat("do_c", 0.0);
  do_calibrated = prefs.getBool("do_cal", false);
  do_cal1_v = prefs.getFloat("do_cal1_v", 1.6);
  do_cal1_t = prefs.getFloat("do_cal1_t", 25.0);
  do_cal2_v = prefs.getFloat("do_cal2_v", 1.0);
  do_cal2_t = prefs.getFloat("do_cal2_t", 25.0);
  do_two_point_mode = prefs.getBool("do_2pt", false);
  
  prefs.end();
  
  Serial.println("📖 Calibration data loaded from NVS");
  Serial.printf("   pH: m=%.3f, c=%.3f, cal=%s\n", ph_regression_m, ph_regression_c, ph_calibrated ? "YES" : "NO");
  Serial.printf("   TDS: m=%.3f, c=%.3f, cal=%s\n", tds_regression_m, tds_regression_c, tds_calibrated ? "YES" : "NO");
  Serial.printf("   DO: m=%.3f, c=%.3f, cal=%s\n", do_regression_m, do_regression_c, do_calibrated ? "YES" : "NO");
}

void loadThresholdData() {
  prefs.begin("elsa_thr", true);
  
  ph_good = prefs.getFloat("ph_good", 6.5);
  ph_bad = prefs.getFloat("ph_bad", 8.0);
  tds_good = prefs.getInt("tds_good", 400);
  tds_bad = prefs.getInt("tds_bad", 800);
  do_good = prefs.getFloat("do_good", 7.0);
  do_bad = prefs.getFloat("do_bad", 4.0);
  
  prefs.end();
  
  Serial.println("📋 Threshold data loaded from NVS");
  Serial.printf("   pH: %.1f - %.1f | TDS: %d - %d | DO: %.1f - %.1f\n", 
                ph_good, ph_bad, tds_good, tds_bad, do_good, do_bad);
}

void saveCalibration() {
  prefs.begin("elsa_cal", false);
  
  prefs.putFloat("ph_m", ph_regression_m);
  prefs.putFloat("ph_c", ph_regression_c);
  prefs.putBool("ph_cal", ph_calibrated);
  
  prefs.putFloat("tds_m", tds_regression_m);
  prefs.putFloat("tds_c", tds_regression_c);
  prefs.putBool("tds_cal", tds_calibrated);
  prefs.putFloat("tds_k", tds_k_value);
  
  prefs.putFloat("do_m", do_regression_m);
  prefs.putFloat("do_c", do_regression_c);
  prefs.putBool("do_cal", do_calibrated);
  prefs.putFloat("do_cal1_v", do_cal1_v);
  prefs.putFloat("do_cal1_t", do_cal1_t);
  prefs.putFloat("do_cal2_v", do_cal2_v);
  prefs.putFloat("do_cal2_t", do_cal2_t);
  prefs.putBool("do_2pt", do_two_point_mode);
  
  prefs.end();
  
  Serial.println("💾 Calibration data saved to NVS");
}

void saveThresholds() {
  prefs.begin("elsa_thr", false);
  
  prefs.putFloat("ph_good", ph_good);
  prefs.putFloat("ph_bad", ph_bad);
  prefs.putInt("tds_good", tds_good);
  prefs.putInt("tds_bad", tds_bad);
  prefs.putFloat("do_good", do_good);
  prefs.putFloat("do_bad", do_bad);
  
  prefs.end();
  
  Serial.println("🎯 Threshold data saved to NVS");
}

#endif
