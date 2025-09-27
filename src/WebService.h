//
// Created by tim on 24.09.25.
//

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <Arduino.h>
#include <AsyncMqttClient.hpp>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include "Task.h"
#include "utils/Macros.h"

class WebService final : public Task {
    SINGLETON(WebService)
public:
    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) const;
    static void onMqttMessage(char *topic, char *payload,
                              AsyncMqttClientMessageProperties properties,
                              size_t len, size_t index, size_t total);

    static String getImagesList();
    static String getCheckedAttribute(bool isChecked);

protected:
    [[noreturn]] void run() override;

private:
    AsyncMqttClient mqttClient;
    AsyncWebServer server;
    Preferences preferences;

    String mqttServer = "";
    int mqttPort = 1883;
    String mqttUser = "";
    String mqttPass = "";
};

#endif //WEBINTERFACE_H
