#include <Arduino.h>
#include <esp_task_wdt.h>
#include "Storage.h"
#include "RTCMod.h"
#include "WifiConf.h"
#include "TempSensor.h"
#include "InitADS.h"
#include "PHSensor.h"
#include "TDSSensor.h"
#include "DOSensor.h"
#include "OffsetSensor.h"
#include "CalibrateSensor.h"
#include "SendData.h"
#include "TFT.h"
#include "Scheduler.h"

float tempC = 0.0;
float ph_raw = 0.0, ph_voltage = 0.0, ph_calibrated_value = 0.0;
float tds_raw = 0.0, tds_voltage = 0.0, tds_calibrated_value = 0.0;
float do_raw = 0.0, do_voltage = 0.0, do_calibrated_value = 0.0;

void setup() {
  Serial.begin(115200);
  delay(3000);

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  initStorage();
  loadCalibrationData();
  loadThresholdData();

  initTFT();
  showStatus("System Init...");
  initRTC();
  initADS();
  initTemperatureSensor();
  initTDSSensor();
  initPHSensor();
  initDOSensor();

  showStatus("Connecting WiFi...");
  connectToWiFi();
  syncTimeFromNTP();

  showStatus("Init MQTT...");
  initSendData();

  addTask("sample", 1000, readAndPublish);
  addTask("header", 5000, updateHeader);
  addTask("display", 1000, refreshDisplayCards);

  showStatus("System Ready!");
  delay(1000);
}

void loop() {
  runTasks();
  client.loop();
  ensureMqttConnection();
  yield();
  esp_task_wdt_reset();
}

void readAndPublish() {
  DateTime now = readRTC();
  
  tempC = readTemperatureC();
  
  readPHSensor(&ph_raw, &ph_voltage, &ph_calibrated_value);
  readTDSSensor(&tds_raw, &tds_voltage, &tds_calibrated_value, tempC);
  readDOSensor(&do_raw, &do_voltage, &do_calibrated_value, tempC);

  Serial.printf("[%02d/%02d/%04d %02d:%02d:%02d] 🌡️ %.2f °C | pH: %.2f | TDS: %.1f ppm | DO: %.2f mg/L\n",
              now.day(), now.month(), now.year(),
              now.hour(), now.minute(), now.second(),
              tempC, ph_calibrated_value, tds_calibrated_value, do_calibrated_value);

  showClock(now);

  sendSensorData(
    ph_raw, ph_voltage, ph_calibrated_value,
    tds_raw, tds_voltage, tds_calibrated_value,
    do_raw, do_voltage, do_calibrated_value,
    tempC
  );
}

void refreshDisplayCards() {
  drawTemp(tempC);
  drawPH(ph_calibrated_value);
  drawTDS(tds_calibrated_value);
  drawDO(do_calibrated_value);
}

float getLastPHCalibrated() { return ph_calibrated_value; }
float getLastTemperature() { return tempC; }