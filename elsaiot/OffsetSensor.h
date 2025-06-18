#ifndef OFFSET_SENSOR_H
#define OFFSET_SENSOR_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "DeviceConfig.h"
#include "Storage.h"

extern PubSubClient client;

extern float ph_good,  ph_bad;
extern int   tds_good, tds_bad;
extern float do_good,  do_bad;

extern float temp_good_low, temp_good_high;

inline bool isTempGood(float v){ return (v >= temp_good_low && v <= temp_good_high); }
inline bool isPHGood  (float v){ return (v >= ph_good      && v <= ph_bad); }
inline bool isTDSGood (float v){ return (v >= tds_good     && v <= tds_bad); }
inline bool isDOGood  (float v){ return (v >= do_good      && v <= do_bad); }

inline uint16_t getTempColor(float v){ return isTempGood(v)?0xFFE0:0xF800; }
inline uint16_t getPHColor  (float v){ return isPHGood  (v)?0x07E0:0xF800; }
inline uint16_t getTDSColor (float v){ return isTDSGood (v)?0x07FF:0xF800; }
inline uint16_t getDOColor  (float v){ return isDOGood  (v)?0xFD20:0xF800; }

inline String getTempStatus(float v){ return isTempGood(v)?F("GOOD"):F("BAD"); }
inline String getPHStatus  (float v){ return isPHGood  (v)?F("GOOD"):F("BAD"); }
inline String getTDSStatus (float v){ return isTDSGood (v)?F("GOOD"):F("BAD"); }
inline String getDOStatus  (float v){ return isDOGood  (v)?F("GOOD"):F("BAD"); }

static void sendThresholdAck(){
    String topic = "elsaiot/" + getDeviceID() + "/ack";

    StaticJsonDocument<512> doc;
    doc["type"] = "threshold";
    JsonObject th = doc.createNestedObject("threshold");

    th["ph_good"]  = ph_good;  th["ph_bad"]  = ph_bad;
    th["tds_good"] = tds_good; th["tds_bad"] = tds_bad;
    th["do_good"]  = do_good;  th["do_bad"]  = do_bad;
    th["temp_low"] = temp_good_low;
    th["temp_high"]= temp_good_high;

    doc["status"]    = "applied";
    doc["timestamp"] = millis();

    char buf[512]; serializeJson(doc, buf);
    client.publish(topic.c_str(), buf);
}

static void handleOffsetMessage(const String& payload){
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload)) return;

    if (!doc.containsKey("threshold")) return;

    JsonObject t = doc["threshold"];

    ph_good  = t["ph_good" ].as<float>();
    ph_bad   = t["ph_bad"  ].as<float>();
    tds_good = t["tds_good"].as<int>();
    tds_bad  = t["tds_bad" ].as<int>();
    do_good  = t["do_good" ].as<float>();
    do_bad   = t["do_bad"  ].as<float>();
    temp_good_low  = t["temp_low" ].as<float>();
    temp_good_high = t["temp_high"].as<float>();

    saveThresholds();
    sendThresholdAck();
}

static inline void subscribeOffset(){
    client.subscribe(("elsaiot/" + getDeviceID() + "/offset").c_str());
}

#endif