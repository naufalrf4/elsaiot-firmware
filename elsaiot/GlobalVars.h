#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

float ph_regression_m = 1.0;
float ph_regression_c = 0.0;
bool ph_calibrated = false;

float tds_regression_m = 1.0;
float tds_regression_c = 0.0;
bool tds_calibrated = false;
float tds_k_value = 1.0;

float do_regression_m = 1.0;
float do_regression_c = 0.0;
bool do_calibrated = false;
float do_cal1_v = 1.6;
float do_cal1_t = 25.0;
float do_cal2_v = 1.0;
float do_cal2_t = 25.0;
bool do_two_point_mode = false;

float ph_good = 6.5;
float ph_bad = 8.0;
int tds_good = 400;
int tds_bad = 800;
float do_good = 7.0;
float do_bad = 4.0;

#endif
