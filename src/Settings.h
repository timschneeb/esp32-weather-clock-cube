//
// Created by tim on 24.09.25.
//

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Preferences.h>
#include <Arduino.h>
#include <atomic>

#include "utils/AtomicValue.h"
#include "utils/Macros.h"

/**
 * X macro: type(name, default)
 * List of all properties with their types and default values.
 * Used to generate member declarations, load and save methods.
 *
 * NOTE: ESP32 NVS keys are limited to 15 characters.
 */
#define FOR_EACH_PROPERTY(STRING,INT,FLOAT) \
    STRING(ssid, "") \
    STRING(pwd, "") \
    STRING(mqtt, "") \
    INT(mqttPort, 1883) \
    STRING(mqttUser, "") \
    STRING(mqttPass, "") \
    INT(displayDuration, 30) \
    STRING(weatherApiKey, "") \
    STRING(weatherCity, "") \
    STRING(weatherIcon, "") \
    STRING(weatherDay, "") \
    FLOAT(weatherHumidity, 0.0f) \
    FLOAT(weatherTempDay, 0.0f) \
    FLOAT(weatherTempMin, 0.0f) \
    FLOAT(weatherTempMax, 0.0f) \
    INT(brightness, 100) \
    STRING(timezone, "CET-1CEST,M3.5.0,M10.5.0/3")

class Settings {
    SINGLETON(Settings)
public:
    void load();
    void save();

    // Member declarations
#define DECL_STRING(name, def) AtomicValue<String> name;
#define DECL_INT(name, def) std::atomic<int> name;
#define DECL_FLOAT(name, def) std::atomic<float> name;
    FOR_EACH_PROPERTY(DECL_STRING, DECL_INT, DECL_FLOAT)
#undef DECL_FLOAT
#undef DECL_INT
#undef DECL_STRING

private:
    Preferences preferences;
};

#endif // SETTINGS_H
