#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <Arduino.h>
#include "PrivateConfig.h"

#define DEVICE_PREFIX "ELSA-"
#define DEFAULT_AP_PASSWORD "12345678"

inline String getDeviceID() {
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);
  String macID = String(chip, HEX);
  macID.toUpperCase();
  return DEVICE_PREFIX + macID;
}

#endif