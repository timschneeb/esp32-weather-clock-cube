//
// Created by tim on 24.09.25.
//

#include "Settings.h"

Settings::Settings() { // NOLINT(*-pro-type-member-init)
    load();
}

void Settings::load() {
    preferences.begin("config", true);
    ssid = preferences.getString("ssid", "");
    pwd = preferences.getString("pwd", "");
    mqtt = preferences.getString("mqtt", "");
    mqttPort = preferences.getInt("mqttport", 1883);
    mqttUser = preferences.getString("mqttuser", "");
    mqttPass = preferences.getString("mqttpass", "");
    fip = preferences.getString("fip", "");
    fport = preferences.getInt("fport", 5000);
    displayDuration = preferences.getInt("sec", 30);
    mode = preferences.getString("mode", "alert");
    weatherApiKey = preferences.getString("weatherApiKey", "");
    weatherCity = preferences.getString("weatherCity", "");
    weatherIcon = preferences.getString("weatherIcon", "");
    weatherDay = preferences.getString("weatherDay", "");
    weatherHumidity = preferences.getFloat("humidity", 0.0);
    weatherTempDay = preferences.getFloat("dayTemp", 0.0);
    weatherTempMin = preferences.getFloat("min", 0.0);
    weatherTempMax = preferences.getFloat("max", 0.0);
    maxImages = preferences.getInt("maxImages", 10);
    brightness = preferences.getInt("brightness", 100);
    timezone = preferences.getString("timezoneName", "CET-1CEST,M3.5.0,M10.5.0/3");
    slideshowInterval = preferences.getInt("slideInterval", 3000);
    preferences.end();
}

void Settings::save() {
    preferences.begin("config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pwd", pwd);
    preferences.putString("mqtt", mqtt);
    preferences.putInt("mqttport", mqttPort);
    preferences.putString("mqttuser", mqttUser);
    preferences.putString("mqttpass", mqttPass);
    preferences.putString("fip", fip);
    preferences.putInt("fport", fport);
    preferences.putInt("sec", displayDuration);
    preferences.putString("mode", mode);
    preferences.putString("weatherApiKey", weatherApiKey);
    preferences.putString("weatherCity", weatherCity);
    preferences.putString("weatherIcon", weatherIcon);
    preferences.putString("weatherDay", weatherDay);
    preferences.putFloat("humidity", weatherHumidity);
    preferences.putFloat("dayTemp", weatherTempDay);
    preferences.putFloat("min", weatherTempMin);
    preferences.putFloat("max", weatherTempMax);
    preferences.putInt("maxImages", maxImages);
    preferences.putInt("brightness", brightness);
    preferences.putString("timezoneName", timezone);
    preferences.putInt("slideInterval", slideshowInterval);
    preferences.end();
}
