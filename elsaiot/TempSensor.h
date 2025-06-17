#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void initTemperatureSensor() {
  sensors.begin();
  Serial.println("🌡️ Temperature sensor initialized.");
}

// === Read Temperature (°C) ===
float readTemperatureC() {
  uint32_t startTime = millis();
  sensors.requestTemperatures();
  
  while (!sensors.isConversionComplete() && millis() - startTime < 1000) {
    yield();
    delay(10);
  }
  
  float tempC = sensors.getTempCByIndex(0);

  if (tempC <= -100 || tempC >= 85) {
    Serial.println("⚠️ Invalid temperature reading detected.");
    return -127.0;
  }

  return tempC;
}

bool isTemperatureSensorConnected() {
  return sensors.getDeviceCount() > 0;
}

uint8_t getTemperatureSensorResolution() {
  DeviceAddress tempDeviceAddress;
  if (sensors.getAddress(tempDeviceAddress, 0)) {
    return sensors.getResolution(tempDeviceAddress);
  }
  return 0;
}

void setTemperatureSensorResolution(uint8_t resolution) {
  DeviceAddress tempDeviceAddress;
  if (sensors.getAddress(tempDeviceAddress, 0)) {
    sensors.setResolution(tempDeviceAddress, resolution);
    Serial.printf("✅ Temperature sensor resolution set to %d bits\n", resolution);
  }
}

#endif