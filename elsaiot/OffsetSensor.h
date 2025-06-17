#ifndef OFFSET_SENSOR_H
#define OFFSET_SENSOR_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "DeviceConfig.h"
#include "Storage.h"

extern PubSubClient client;
extern float ph_good;
extern float ph_bad;
extern int tds_good;
extern int tds_bad;
extern float do_good;
extern float do_bad;

inline bool isPHGood(float value) {
  return (value >= ph_good && value <= ph_bad);
}

inline bool isTDSGood(float value) {
  return (value >= tds_good && value <= tds_bad);
}

inline bool isDOGood(float value) {
  return (value >= do_good && value <= do_bad);
}

inline uint16_t getPHColor(float value) {
  return isPHGood(value) ? 0x07E0 : 0xF800; // GREEN : RED
}

inline uint16_t getTDSColor(float value) {
  return isTDSGood(value) ? 0x07FF : 0xF800; // CYAN : RED
}

inline uint16_t getDOColor(float value) {
  return isDOGood(value) ? 0xFD20 : 0xF800; // ORANGE : RED
}

inline String getPHStatus(float value) {
  return isPHGood(value) ? "GOOD" : "BAD";
}

inline String getTDSStatus(float value) {
  return isTDSGood(value) ? "GOOD" : "BAD";
}

inline String getDOStatus(float value) {
  return isDOGood(value) ? "GOOD" : "BAD";
}

void sendThresholdAck() {
  String topic = "elsaiot/" + getDeviceID() + "/ack";
  
  StaticJsonDocument<768> doc;
  doc["type"] = "threshold";
  doc["threshold"]["ph_good"] = ph_good;
  doc["threshold"]["ph_bad"] = ph_bad;
  doc["threshold"]["tds_good"] = tds_good;
  doc["threshold"]["tds_bad"] = tds_bad;
  doc["threshold"]["do_good"] = do_good;
  doc["threshold"]["do_bad"] = do_bad;
  doc["status"] = "applied";
  doc["timestamp"] = millis();
  
  char buffer[768];
  serializeJson(doc, buffer);
  
  client.publish(topic.c_str(), buffer);
  Serial.println("📤 Sent threshold acknowledgment");
}

void handleOffsetMessage(const String& payload) {
  StaticJsonDocument<768> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("❌ Failed to parse offset payload");
    return;
  }

  bool updated = false;

  if (doc.containsKey("threshold")) {
    ph_good = doc["threshold"]["ph_good"];
    ph_bad = doc["threshold"]["ph_bad"];
    tds_good = doc["threshold"]["tds_good"];
    tds_bad = doc["threshold"]["tds_bad"];
    do_good = doc["threshold"]["do_good"];
    do_bad = doc["threshold"]["do_bad"];

    Serial.println("✅ Threshold updated successfully");
    updated = true;
  }

  if (updated) {
    saveThresholds();
    sendThresholdAck();
  }
}

void subscribeOffset() {
  String topic = "elsaiot/" + getDeviceID() + "/offset";
  client.subscribe(topic.c_str());
  Serial.println("📡 Subscribed to topic: " + topic);
}

#endif
