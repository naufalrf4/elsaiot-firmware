#ifndef CALIBRATE_SENSOR_H
#define CALIBRATE_SENSOR_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "DeviceConfig.h"
#include "Storage.h"
#include "DOSensor.h"
#include "TDSSensor.h"

extern PubSubClient client;
extern bool         calibrationReady;

static void sendCalibrationAck() {
    if (!client.connected()) return;

    String topic = "elsaiot/" + getDeviceID() + "/ack/calibrate";

    StaticJsonDocument<512> doc;
    doc["timestamp"] = millis();
    doc["message"]   = "Calibration applied";

    JsonObject st = doc.createNestedObject("status");

    JsonObject phSt = st.createNestedObject("ph");
    phSt["calibrated"] = ph_calibrated;

    JsonObject tdsSt = st.createNestedObject("tds");
    tdsSt["calibrated"] = tds_calibrated;
    tdsSt["k"]          = tds_k_value;

    JsonObject doSt = st.createNestedObject("do");
    doSt["calibrated"] = do_calibrated;
    doSt["mode"]       = do_calibrated ? (do_two_point_mode ? "two-point" : "single-point") : "none";

    char buf[512];
    serializeJson(doc, buf);
    client.publish(topic.c_str(), buf);
}

static void handleCalibrateMessage(const String &payload) {
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload)) return;

    bool changed = false;

    if (doc.containsKey("ph")) {
        ph_regression_m = doc["ph"]["m"];
        ph_regression_c = doc["ph"]["c"];
        ph_calibrated   = true;
        changed         = true;
    }

    if (doc.containsKey("tds")) {
        JsonObject t = doc["tds"];
        float v   = t["v"]   | -1.0;
        float std = t["std"] | -1.0;
        float tp  = t["t"]   | -100.0;
        if (v > 0 && std > 0 && tp > -50) {
            applyTDSCalibration(v, std, tp);
            changed = true;
        }
    }

    if (doc.containsKey("do")) {
        JsonObject d = doc["do"];
        const char* mode = d["mode"] | "single";

        float v1 = d["v1"] | -1.0;
        float t1 = d["t1"] | -100.0;

        if (v1 > 0 && t1 > -50) {
            if (strcmp(mode, "double") == 0) {
                float v2 = d["v2"] | -1.0;
                float t2 = d["t2"] | -100.0;
                if (v2 > 0 && t2 > -50) {
                    applyTwoPointDOCalibration(v1, t1, v2, t2);
                    changed = true;
                }
            } else {
                applySinglePointDOCalibration(v1, t1);
                changed = true;
            }
        }
    }

    if (changed) {
        calibrationReady = true;
        saveCalibration();
        sendCalibrationAck();
    }
}

static void subscribeCalibrate() {
    client.subscribe(("elsaiot/" + getDeviceID() + "/calibrate").c_str());
}

#endif