//
// Created by tim on 24.09.25.
//

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <Arduino.h>
#include <AsyncMqttClient.hpp>

#include "Task.h"
#include "WebApi.h"
#include "utils/Macros.h"

class WebService final : public Task {
    SINGLETON(WebService)
public:
    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) const;
    static void onMqttMessage(char* topic, char* payload,
        AsyncMqttClientMessageProperties properties,
        size_t len, size_t index, size_t total);

protected:
    [[noreturn]] void run() override;

private:
    AsyncMqttClient mqttClient;
    WebApi api;

    void setupMqttFromSettings();
};

#endif // WEBINTERFACE_H
