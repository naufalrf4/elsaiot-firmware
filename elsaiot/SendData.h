#ifndef SEND_DATA_H
#define SEND_DATA_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DeviceConfig.h"
#include "Storage.h"
#include "RTCMod.h"

WiFiClient espClient;
PubSubClient client(espClient);

uint32_t lastMqttConnectionAttempt = 0;
const uint32_t MQTT_RETRY_INTERVAL = 10000;
bool mqttConnectionInProgress = false;
bool calibrationReady = false;

void handleCalibrateMessage(const String& payload);
void handleOffsetMessage(const String& payload);
void subscribeCalibrate();
void subscribeOffset();

String getPHStatus(float value);
String getTDSStatus(float value);
String getDOStatus(float value);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String incoming = "";
  for (unsigned int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }
  
  String deviceId = getDeviceID();
  String calibrateTopic = "elsaiot/" + deviceId + "/calibrate";
  String offsetTopic = "elsaiot/" + deviceId + "/offset";
  
  if (String(topic) == calibrateTopic) {
    handleCalibrateMessage(incoming);
  } else if (String(topic) == offsetTopic) {
    handleOffsetMessage(incoming);
  }
}

void initSendData() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);
  client.setBufferSize(1024);
  Serial.println("📡 MQTT client initialized");
}

void sendBootNotification() {
  String topic = "elsaiot/" + getDeviceID() + "/status";
  StaticJsonDocument<512> doc;
  doc["event"] = "boot";
  doc["device_id"] = getDeviceID();
  doc["timestamp"] = formatTimestamp(readRTC());

  char buffer[512];
  serializeJson(doc, buffer);
  client.publish(topic.c_str(), buffer);
}

bool ensureMqttConnection() {
  static uint32_t lastConnectionCheck = 0;
  uint32_t now = millis();

  if (now - lastConnectionCheck < 1000) {
    return client.connected();
  }
  lastConnectionCheck = now;

  if (client.connected()) {
    return true;
  }

  if (now - lastMqttConnectionAttempt < MQTT_RETRY_INTERVAL) {
    return false;
  }

  lastMqttConnectionAttempt = now;
  String clientId = getDeviceID();

  if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("✅ MQTT connected");
    subscribeCalibrate();
    subscribeOffset();
    sendBootNotification();
    return true;
  } else {
    Serial.printf("❌ MQTT failed, rc=%d\n", client.state());
    return false;
  }
}

void sendSensorData(
    float ph_raw, float ph_voltage, float ph_calibrated,
    float tds_raw, float tds_voltage, float tds_calibrated,
    float do_raw, float do_voltage, float do_calibrated,
    float temperature_value
) {
  if (!client.connected()) return;

  String topic = "elsaiot/" + getDeviceID() + "/data";
  StaticJsonDocument<1024> doc;

  doc["ph"]["raw"]           = ph_raw;
  doc["ph"]["voltage"]       = ph_voltage;
  doc["ph"]["calibrated"]    = ph_calibrated;
  doc["ph"]["calibrated_ok"] = ph_calibrated;
  doc["ph"]["status"]        = getPHStatus(ph_calibrated);

  doc["tds"]["raw"]           = tds_raw;
  doc["tds"]["voltage"]       = tds_voltage;
  doc["tds"]["calibrated"]    = tds_calibrated;
  doc["tds"]["calibrated_ok"] = tds_calibrated;
  doc["tds"]["status"]        = getTDSStatus(tds_calibrated);

  doc["do"]["raw"]            = do_raw;
  doc["do"]["voltage"]        = do_voltage;
  doc["do"]["calibrated"]     = do_calibrated;
  doc["do"]["calibrated_ok"]  = do_calibrated;
  doc["do"]["status"]         = getDOStatus(do_calibrated);

  doc["temperature"]["value"]  = temperature_value;
  doc["temperature"]["status"] = getTempStatus(temperature_value);

  doc["timestamp"] = formatTimestamp(readRTC());

  char buffer[1024];
  serializeJson(doc, buffer);
  client.publish(topic.c_str(), buffer);
}

#endif
