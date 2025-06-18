#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include <Arduino.h>
#include "GlobalVars.h"

Preferences prefs;

extern float ph_regression_m, ph_regression_c;  extern bool  ph_calibrated;
extern float tds_k_value;                       extern bool  tds_calibrated;
extern bool  do_calibrated;                     extern float do_cal1_v, do_cal1_t, do_cal2_v, do_cal2_t; extern bool do_two_point_mode;
extern bool  calibrationReady;

extern float ph_good, ph_bad;   extern int tds_good, tds_bad;   extern float do_good, do_bad;
extern float temp_good_low, temp_good_high;

static void initStorage() {
    ph_regression_m = 1.0f; ph_regression_c = 0.0f; ph_calibrated = false;

    tds_k_value     = 1.0f; tds_calibrated  = false;

    do_calibrated   = false;
    do_cal1_v = 1.6f; do_cal1_t = 25.0f;
    do_cal2_v = 1.0f; do_cal2_t = 25.0f;
    do_two_point_mode = false;

    calibrationReady = false;

    ph_good  = 6.5f; ph_bad  = 8.0f;
    tds_good = 400;  tds_bad = 800;
    do_good  = 7.0f; do_bad  = 4.0f;

    temp_good_low  = 20.0f;
    temp_good_high = 30.0f;
}

static void loadCalibrationData() {
    prefs.begin("elsa_cal", true);

    ph_regression_m  = prefs.getFloat("ph_m", 1.0f);
    ph_regression_c  = prefs.getFloat("ph_c", 0.0f);
    ph_calibrated    = prefs.getBool ("ph_cal", false);

    tds_k_value      = prefs.getFloat("tds_k", 1.0f);
    tds_calibrated   = prefs.getBool ("tds_cal", false);

    do_calibrated     = prefs.getBool ("do_cal", false);
    do_cal1_v         = prefs.getFloat("do_cal1_v", 1.6f);
    do_cal1_t         = prefs.getFloat("do_cal1_t", 25.0f);
    do_cal2_v         = prefs.getFloat("do_cal2_v", 1.0f);
    do_cal2_t         = prefs.getFloat("do_cal2_t", 25.0f);
    do_two_point_mode = prefs.getBool ("do_2pt", false);

    prefs.end();

    calibrationReady = ph_calibrated || tds_calibrated || do_calibrated;
}

static void loadThresholdData() {
    prefs.begin("elsa_thr", true);

    ph_good  = prefs.getFloat("ph_good", 6.5f);
    ph_bad   = prefs.getFloat("ph_bad", 8.0f);
    tds_good = prefs.getInt  ("tds_good", 400);
    tds_bad  = prefs.getInt  ("tds_bad", 800);
    do_good  = prefs.getFloat("do_good", 7.0f);
    do_bad   = prefs.getFloat("do_bad", 4.0f);

    temp_good_low  = prefs.getFloat("temp_low",  20.0f);
    temp_good_high = prefs.getFloat("temp_high", 30.0f);

    prefs.end();
}

static void saveCalibration() {
    prefs.begin("elsa_cal", false);

    prefs.putFloat("ph_m",  ph_regression_m);
    prefs.putFloat("ph_c",  ph_regression_c);
    prefs.putBool ("ph_cal",ph_calibrated);

    prefs.putFloat("tds_k",  tds_k_value);
    prefs.putBool ("tds_cal",tds_calibrated);

    prefs.putBool ("do_cal", do_calibrated);
    prefs.putFloat("do_cal1_v", do_cal1_v);
    prefs.putFloat("do_cal1_t", do_cal1_t);
    prefs.putFloat("do_cal2_v", do_cal2_v);
    prefs.putFloat("do_cal2_t", do_cal2_t);
    prefs.putBool ("do_2pt", do_two_point_mode);

    prefs.end();

    calibrationReady = ph_calibrated || tds_calibrated || do_calibrated;
}

static void saveThresholds() {
    prefs.begin("elsa_thr", false);

    prefs.putFloat("ph_good",  ph_good);
    prefs.putFloat("ph_bad",   ph_bad);
    prefs.putInt  ("tds_good", tds_good);
    prefs.putInt  ("tds_bad",  tds_bad);
    prefs.putFloat("do_good",  do_good);
    prefs.putFloat("do_bad",   do_bad);

    prefs.putFloat("temp_low",  temp_good_low);
    prefs.putFloat("temp_high", temp_good_high);

    prefs.end();
}

static void clearCalibrationNVS() {
    prefs.begin("elsa_cal", false);
    prefs.clear();
    prefs.end();
    initStorage();
}

#endif