#ifndef CALIBRATE_SENSOR_H
#define CALIBRATE_SENSOR_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "DeviceConfig.h"
#include "Storage.h"

extern PubSubClient client;
extern bool calibrationReady;

void sendCalibrationAck() {
  if (!client.connected()) return;
  
  String topic = "elsaiot/" + getDeviceID() + "/ack/calibrate";
  
  StaticJsonDocument<768> doc;
  doc["timestamp"] = millis();
  doc["message"] = "Calibration applied successfully";
  doc["status"]["ph"]["calibrated"] = ph_calibrated;
  doc["status"]["tds"]["calibrated"] = tds_calibrated;
  doc["status"]["do"]["calibrated"] = do_calibrated;
  
  char buffer[768];
  serializeJson(doc, buffer);
  
  client.publish(topic.c_str(), buffer);
  Serial.println("📡 Calibration acknowledgment sent");
}

void handleCalibrateMessage(const String& payload) {
  StaticJsonDocument<768> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("❌ Failed to parse calibrate payload");
    return;
  }

  bool calibration_applied = false;

  if (doc.containsKey("ph")) {
    ph_regression_m = doc["ph"]["m"];
    ph_regression_c = doc["ph"]["c"];
    ph_calibrated = true;
    calibration_applied = true;
    Serial.printf("✅ PH Calibrated: m=%.3f, c=%.3f\n", ph_regression_m, ph_regression_c);
  }

  if (doc.containsKey("tds")) {
    tds_regression_m = doc["tds"]["m"];
    tds_regression_c = doc["tds"]["c"];
    tds_calibrated = true;
    calibration_applied = true;
    Serial.printf("✅ TDS Calibrated: m=%.3f, c=%.3f\n", tds_regression_m, tds_regression_c);
  }

  if (doc.containsKey("do")) {
    do_regression_m = doc["do"]["m"];
    do_regression_c = doc["do"]["c"];
    do_calibrated = true;
    calibration_applied = true;
    Serial.printf("✅ DO Calibrated: m=%.3f, c=%.3f\n", do_regression_m, do_regression_c);
  }

  if (calibration_applied) {
    calibrationReady = true;
    saveCalibration();
    sendCalibrationAck();
  }
}

void subscribeCalibrate() {
  String topic = "elsaiot/" + getDeviceID() + "/calibrate";
  client.subscribe(topic.c_str());
  Serial.println("📡 Subscribed to topic: " + topic);
}

#endif
