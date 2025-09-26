//
// Created by tim on 24.09.25.
//

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <Arduino.h>
#include <AsyncMqttClient.hpp>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include "utils/Macros.h"

class WebServer final {
    SINGLETON(WebServer)
public:
    void onMqttConnect(bool sessionPresent);

    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);

    static void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len,
                              size_t index,
                              size_t total);

    static String getImagesList();
    static String getCheckedAttribute(bool isChecked);

    void run(void *pvParameters);

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
