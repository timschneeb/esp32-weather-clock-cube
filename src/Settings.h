//
// Created by tim on 24.09.25.
//

#ifndef SETTINGS_H
#define SETTINGS_H

#include <atomic>
#include <Preferences.h>

#include "utils/AtomicValue.h"
#include "utils/Macros.h"

class Settings {
    SINGLETON(Settings)
public:
    void load();
    void save();

#define DECL_PRIMITIVE_PROPERTY(type, name) std::atomic<type> name;
#define DECL_PROPERTY(type, name) AtomicValue<type> name;
    DECL_PROPERTY(String, ssid)
    DECL_PROPERTY(String, pwd)
    DECL_PROPERTY(String, mqtt)
    DECL_PRIMITIVE_PROPERTY(int, mqttPort)
    DECL_PROPERTY(String, mqttUser)
    DECL_PROPERTY(String, mqttPass)
    DECL_PROPERTY(String, fip)
    DECL_PRIMITIVE_PROPERTY(int, fport)
    DECL_PRIMITIVE_PROPERTY(int, displayDuration)
    DECL_PROPERTY(String, mode)
    DECL_PROPERTY(String, weatherApiKey)
    DECL_PROPERTY(String, weatherCity)
    DECL_PROPERTY(String, weatherIcon)
    DECL_PROPERTY(String, weatherDay)
    DECL_PRIMITIVE_PROPERTY(float, weatherHumidity)
    DECL_PRIMITIVE_PROPERTY(float, weatherTempDay)
    DECL_PRIMITIVE_PROPERTY(float, weatherTempMin)
    DECL_PRIMITIVE_PROPERTY(float, weatherTempMax)
    DECL_PRIMITIVE_PROPERTY(int, maxImages)
    DECL_PRIMITIVE_PROPERTY(int, brightness)
    DECL_PROPERTY(String, timezone)
    DECL_PRIMITIVE_PROPERTY(int, slideshowInterval)
#undef DECL_PROPERTY
#undef DECL_PRIMITIVE_PROPERTY

private:
    Preferences preferences;
};



#endif //SETTINGS_H
