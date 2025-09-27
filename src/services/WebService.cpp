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

using namespace ArduinoJson;

auto MQTT_TOPIC = "frigate/reviews";
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
    Serial.println("[MQTT] Connected!");
    mqttClient.subscribe(MQTT_TOPIC, 0);
    vTaskSuspend(handle());
}

void WebService::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) const
{
    Serial.print("[MQTT] Connection lost with reason: ");
    Serial.println(static_cast<int>(reason));
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
    Serial.println("====[MQTT RECEIVED]====");
    Serial.print("Topic:   ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(payloadStr);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, len);
    if (error) {
        Serial.print("[DEBUG] JSON parsing error: ");
        Serial.println(error.c_str());
        EventBus::instance().publish<WEB_MqttErrorEvent>("JSON Parse Error:\n" + String(error.c_str()));
        return;
    }

    String type = doc["type"] | "";
    JsonObject msg;
    String severity = "";

    if (type == "new" && doc["before"].is<JsonObject>()) {
        msg = doc["before"].as<JsonObject>();
    } else if (type == "update" && doc["after"].is<JsonObject>()) {
        msg = doc["after"].as<JsonObject>();
    } else {
        Serial.println("[DEBUG] Ignoring message of type '" + type + "'");
        return;
    }

    if (msg["severity"].is<String>())
        severity = msg["severity"].as<String>();

    String frigateIp = Settings::instance().fip;
    int frigatePort = Settings::instance().fport;
    String mode = Settings::instance().mode;
    mode.trim();
    mode.toLowerCase();
    severity.trim();
    severity.toLowerCase();
    bool show = mode.indexOf(severity) >= 0;

    if (show && frigateIp.length() > 0 && msg["data"].is<JsonObject>() && msg["data"]["detections"].is<JsonArray>()) {
        auto detections = msg["data"]["detections"].as<JsonArray>();
        auto zonesArray = msg["data"]["zones"].is<JsonArray>()
            ? msg["data"]["zones"].as<JsonArray>()
            : JsonArray();
        String zone = "outside-zone";
        if (!zonesArray.isNull() && zonesArray.size() > 0) {
            zone = String(zonesArray[zonesArray.size() - 1].as<const char*>());
        }

        if (!detections.isNull() && detections.size() > 0) {
            for (JsonVariant d : detections) {
                auto id = d.as<String>();
                int dashIndex = id.indexOf("-");
                String suffix = (dashIndex > 0) ? id.substring(dashIndex + 1) : id;
                String filename = "/events/" + suffix + "-" + zone + ".jpg";
                EventBus::instance().publish<WEB_ShowLocalImageEvent>(filename);
            }
            String url = "http://" + frigateIp + ":" + String(frigatePort) + "/api/events/" + detections[0].as<String>() + "/snapshot.jpg?crop=1&height=240";
            EventBus::instance().publish<WEB_ShowImageFromUrlWithZoneEvent>(url, zone);
        }
    }
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
