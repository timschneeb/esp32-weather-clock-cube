//
// Created by tim on 24.09.25.
//

#include "WebService.h"

#include <ArduinoJson.h>
#include <Update.h>

#include "NetworkService.h"
#include "Settings.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "utils/Macros.h"

using namespace ArduinoJson;

auto MQTT_TOPIC = "weathercube/main";
auto CLIENT_ID = "ESP32Client";

WebService::WebService(): Task("WebServices", 4096, 1) {
    mqttClient.onConnect(std::bind(&WebService::onMqttConnect, this, std::placeholders::_1));
    mqttClient.onDisconnect(std::bind(&WebService::onMqttDisconnect, this, std::placeholders::_1));
    mqttClient.onMessage(&WebService::onMqttMessage);
    setupMqttFromSettings();

    api.setOnMqttConfigChanged([this](const String& server, const int port, const String& user, const String& pass) {
        if (mqttClient.connected())
            mqttClient.disconnect();
        mqttClient.setServer(server.c_str(), port);
        mqttClient.setCredentials(user.c_str(), pass.c_str());
        mqttClient.connect();
    });

    if (NetworkService::isConnected()) {
        mqttClient.connect();
    }
}

void WebService::setupMqttFromSettings() {
    const auto& settings = Settings::instance();
    mqttClient.setServer(settings.mqtt.load().c_str(), settings.mqttPort);
    mqttClient.setCredentials(settings.mqttUser.load().c_str(), settings.mqttPass.load().c_str());
}

void WebService::onMqttConnect(bool sessionPresent)
{
    LOG_INFO("MQTT connected!");
    mqttClient.subscribe(MQTT_TOPIC, 0);
    vTaskSuspend(handle());
}

void WebService::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) const
{
    LOG_ERROR("MQTT connection lost with reason: %d", static_cast<int>(reason));
    EventBus::instance().publish<WEB_MqttDisconnectedEvent>(reason);
    vTaskResume(handle());
}

void WebService::onMqttMessage(
    char* topic,
    char* payload,
    AsyncMqttClientMessageProperties properties,
    size_t len,
    size_t index,
    size_t total)
{
    String payloadStr;
    for (size_t i = 0; i < len; i++)
        payloadStr += payload[i];
    LOG_DEBUG("====[MQTT RECEIVED]====");
    LOG_DEBUG("Topic:   %s", topic);
    LOG_DEBUG("Payload: %s", payloadStr.c_str());

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, len);
    if (error) {
        LOG_ERROR("JSON parsing error: %s", error.c_str());
        EventBus::instance().publish<WEB_MqttErrorEvent>("JSON Parse Error:\n" + String(error.c_str()));
        return;
    }

    // Currently unused
    (void)0;
}

[[noreturn]] void WebService::run()
{
    for (;;) {
        if (!mqttClient.connected()) {
            setupMqttFromSettings();
            mqttClient.connect();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
