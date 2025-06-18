#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

float ph_regression_m = 1.0f;
float ph_regression_c = 0.0f;
bool  ph_calibrated   = false;

float tds_k_value     = 1.0f;
bool  tds_calibrated  = false;

bool  do_calibrated      = false;
float do_cal1_v          = 1.6f;   // mV  @ do_cal1_t
float do_cal1_t          = 25.0f;  // °C
float do_cal2_v          = 1.0f;   // mV  @ do_cal2_t
float do_cal2_t          = 25.0f;  // °C
bool  do_two_point_mode  = false;

float ph_good  = 6.5f;
float ph_bad   = 8.0f;
int   tds_good = 400;
int   tds_bad  = 800;
float do_good  = 7.0f;
float do_bad   = 4.0f;
float temp_good_low  = 20.0f;
float temp_good_high = 30.0f;

#endif