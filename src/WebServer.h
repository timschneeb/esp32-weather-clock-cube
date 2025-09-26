//
// Created by tim on 24.09.25.
//

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <AsyncMqttClient.hpp>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <QuarkTS.h>

class WebServer final : public qOS::task {
public:
    WebServer();

    void onMqttConnect(bool sessionPresent);

    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);

    static void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len,
                              size_t index,
                              size_t total);

    static String getImagesList();
    static String getCheckedAttribute(bool isChecked);

protected:
    void activities(qOS::event_t e) override;

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
