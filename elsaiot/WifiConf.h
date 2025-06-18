#ifndef WIFI_CONF_H
#define WIFI_CONF_H

#include <WiFi.h>
#include <WiFiManager.h>
#include "DeviceConfig.h"

#define AP_TIMEOUT_SEC        180
#define WIFI_CONNECT_TIMEOUT   20
#define RESET_PIN               0 

bool wifi_debug = true;

void connectToWiFi() {
  WiFiManager wm;

  String apSSID = getDeviceID();

  if (wifi_debug) {
    Serial.println("📡 Launching WiFiManager with SSID: " + apSSID);
  }

  wm.setDebugOutput(wifi_debug);
  wm.setConfigPortalTimeout(AP_TIMEOUT_SEC);
  wm.setConnectTimeout(WIFI_CONNECT_TIMEOUT);

  pinMode(RESET_PIN, INPUT_PULLUP);
  if (digitalRead(RESET_PIN) == LOW) {
    wm.resetSettings();
    if (wifi_debug) Serial.println("🔄 Reset WiFi settings via button");
    delay(1000);
  }

  bool res = wm.autoConnect(apSSID.c_str(), DEFAULT_AP_PASSWORD);

  if (!res) {
    Serial.println("❌ WiFi failed. Rebooting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("✅ WiFi Connected");
  Serial.println("📶 IP Address: " + WiFi.localIP().toString());
  Serial.println("📶 MAC Address: " + WiFi.macAddress());
  Serial.printf("📶 Signal Strength: %d dBm\n", WiFi.RSSI());
}

inline bool wifiIsOK() {
  return WiFi.status() == WL_CONNECTED;
}

inline int wifiRSSI() {
  return wifiIsOK() ? WiFi.RSSI() : -1000;
}

bool checkWiFiHealth() {
  if (!wifiIsOK()) {
    Serial.println("⚠️ WiFi connection lost");
    return false;
  }
  
  int rssi = WiFi.RSSI();
  if (rssi < -90) {
    Serial.printf("⚠️ Weak WiFi signal: %d dBm\n", rssi);
    return false;
  }
  
  return true;
}

void reconnectWiFi() {
  static uint32_t lastReconnectAttempt = 0;
  uint32_t now = millis();
  
  if (now - lastReconnectAttempt < 30000) {
    return;
  }
  lastReconnectAttempt = now;
  
  Serial.println("🔄 Attempting WiFi reconnection...");
  WiFi.reconnect();
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 100) {
    delay(100);
    attempts++;
    yield();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi reconnected successfully");
  } else {
    Serial.println("❌ WiFi reconnection failed");
  }
}

#endif