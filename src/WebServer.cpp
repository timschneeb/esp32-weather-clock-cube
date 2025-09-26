//
// Created by tim on 24.09.25.
//

#include "WebServer.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Update.h>

#include "Config.h"
#include "EventBus.h"
#include "Events.h"
#include "NetworkService.h"
#include "Settings.h"

using namespace ArduinoJson;

auto MQTT_TOPIC = "frigate/reviews";
auto CLIENT_ID = "ESP32Client";

WebServer::WebServer() : server(80) {
    preferences.begin("config", false);
    mqttServer = preferences.getString("mqtt", "");
    mqttPort = preferences.getInt("mqttport", 1883);
    mqttUser = preferences.getString("mqttuser", "");
    mqttPass = preferences.getString("mqttpass", "");
    preferences.end();

    mqttClient.onConnect(std::bind(&WebServer::onMqttConnect, this, std::placeholders::_1));
    mqttClient.onDisconnect(std::bind(&WebServer::onMqttDisconnect, this, std::placeholders::_1));
    mqttClient.onMessage(&WebServer::onMqttMessage);
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCredentials(mqttUser.c_str(), mqttPass.c_str());
    if (NetworkService::isConnected()) {
        mqttClient.connect();
    }

    server.serveStatic("/styles.css", SPIFFS, "/styles.css");
    server.serveStatic("/scripts.js", SPIFFS, "/scripts.js");
    server.serveStatic("/events", SPIFFS, "/events");
    server.serveStatic("/icons", SPIFFS, "/icons");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        const auto config = &Settings::instance();
        String mode = config->mode;
        mode.toLowerCase();
        const bool isAlertChecked = mode.indexOf("alert") >= 0;
        const bool isDetectionChecked = mode.indexOf("detection") >= 0;

        const String alertCheckbox = "<input type='checkbox' id='mode_alert' name='mode_alert' value='alert' " +
                               getCheckedAttribute(isAlertChecked) + ">";
        const String detectionCheckbox = "<input type='checkbox' id='mode_detection' name='mode_detection' value='detection' "
                                   + getCheckedAttribute(isDetectionChecked) + ">";

        File file = SPIFFS.open("/index.html", "r");
        if (!file) {
            request->send(500, "text/plain", "Could not open index.html");
            return;
        }
        String html = file.readString();
        file.close();

        // Fill HTML placeholders
        html.replace("{{ssid}}", config->ssid);
        html.replace("{{pwd}}", config->pwd.load() != "" ? "******" : "");
        html.replace("{{pwd_exists}}", config->pwd.load() != "" ? "1" : "0");
        html.replace("{{mqtt}}", config->mqtt);
        html.replace("{{mqttport}}", String(config->mqttPort));
        html.replace("{{mqttuser}}", config->mqttUser);
        html.replace("{{mqttpass}}", config->mqttPass.load() != "" ? "******" : "");
        html.replace("{{mqttpass_exists}}", config->mqttPass.load() != "" ? "1" : "0");
        html.replace("{{fip}}", config->fip);
        html.replace("{{fport}}", String(config->fport));
        html.replace("{{sec}}", String(config->displayDuration));
        html.replace("{{maxImages}}", String(config->maxImages));
        html.replace("{{slideshowInterval}}", String(config->slideshowInterval));
        html.replace("{{alertCheckbox}}", alertCheckbox);
        html.replace("{{detectionCheckbox}}", detectionCheckbox);
        html.replace("{{weatherApiKey}}", config->weatherApiKey.load() != "" ? "******" : "");
        html.replace("{{weatherApiKey_exists}}", config->weatherApiKey.load() != "" ? "1" : "0");
        html.replace("{{weatherCity}}", config->weatherCity);
        html.replace("{{timezone}}", String(config->timezone));
        html.replace("{{brightness}}", String(config->brightness));
        html.replace("{{totalBytes}}", String(SPIFFS.totalBytes() / 1024));
        html.replace("{{usedBytes}}", String(SPIFFS.usedBytes() / 1024));
        html.replace("{{freeBytes}}", String((SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024));
        html.replace("{{imagesList}}", getImagesList());

        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
        response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "-1");
        request->send(response);
    });

    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request) {
        // Safe integer fetch helper
        auto getIntParam = [](AsyncWebServerRequest *req, const char *name, int def) -> int {
            if (req->getParam(name, true)) {
                const String v = req->getParam(name, true)->value();
                if (v.length() > 0) return v.toInt();
            }
            return def;
        };

        preferences.begin("config", false);
        String currentPwd = preferences.getString("pwd", "");
        String currentMqttPass = preferences.getString("mqttpass", "");
        String currentApiKey = preferences.getString("weatherApiKey", "");

        String newSSID = request->getParam("ssid", true)->value();
        preferences.putString("ssid", newSSID);

        String newPwd = request->getParam("pwd", true)->value();
        bool pwdExists = request->getParam("pwd_exists", true)->value() == "1";
        if (!pwdExists || newPwd != "******") {
            preferences.putString("pwd", newPwd);
        }

        String newMqtt = request->getParam("mqtt", true)->value();
        preferences.putString("mqtt", newMqtt);

        int newMqttPort = getIntParam(request, "mqttport", 0);
        if (newMqttPort < 1 || newMqttPort > 65535) newMqttPort = 1883;
        preferences.putInt("mqttport", newMqttPort);

        String newMqttUser = request->getParam("mqttuser", true)->value();
        preferences.putString("mqttuser", newMqttUser);

        String newMqttPass = request->getParam("mqttpass", true)->value();
        bool mqttPassExists = request->getParam("mqttpass_exists", true)->value() == "1";
        if (!mqttPassExists || newMqttPass != "******") {
            preferences.putString("mqttpass", newMqttPass);
        }

        String newFip = request->getParam("fip", true)->value();
        preferences.putString("fip", newFip);

        int newFport = getIntParam(request, "fport", 0);
        if (newFport < 1 || newFport > 65535) newFport = 5000;
        preferences.putInt("fport", newFport);

        int newSec = getIntParam(request, "sec", 0);
        if (newSec < 1) newSec = 30;
        preferences.putInt("sec", newSec);

        int newMaxImages = getIntParam(request, "maxImages", 0);
        if (newMaxImages < 1) newMaxImages = 1;
        if (newMaxImages > 60) newMaxImages = 60;
        preferences.putInt("maxImages", newMaxImages);

        int newSlideshowInterval = getIntParam(request, "slideshowInterval", 0);
        if (newSlideshowInterval < 500) newSlideshowInterval = 3000;
        if (newSlideshowInterval > 20000) newSlideshowInterval = 20000;
        preferences.putInt("slideInterval", newSlideshowInterval);

        // Modes
        String modeValue = "";
        if (request->hasParam("mode_alert", true)) {
            if (request->getParam("mode_alert", true)->value() == "alert") {
                modeValue += "alert";
            }
        }
        if (request->hasParam("mode_detection", true)) {
            if (request->getParam("mode_detection", true)->value() == "detection") {
                if (modeValue != "") modeValue += ",";
                modeValue += "detection";
            }
        }
        preferences.putString("mode", modeValue);

        // Weather
        String newApiKey = request->getParam("weatherApiKey", true)->value();
        bool apiKeyExists = request->getParam("weatherApiKey_exists", true)->value() == "1";
        if (!apiKeyExists || newApiKey != "******") {
            preferences.putString("weatherApiKey", newApiKey);
            EventBus::instance().publish<CFG_WeatherUpdatedEvent>();
        }
        String newCity = request->getParam("weatherCity", true)->value();
        preferences.putString("weatherCity", newCity);

        auto newTimezone = request->getParam("timezoneName", true)->value();
        preferences.putString("timezoneName", newTimezone);

        // Brightness
        int newBrightness = getIntParam(request, "brightness", 0);
        if (newBrightness >= 0 && newBrightness <= 255) {
            preferences.putInt("brightness", newBrightness);
        }

        bool mqttConfigChanged = newMqtt != mqttServer || newMqttPort != mqttPort || newMqttUser != mqttUser || (
                                     newMqttPass != "******" && newMqttPass != mqttPass);
        mqttServer = newMqtt;
        mqttPort = newMqttPort;
        mqttUser = newMqttUser;
        mqttPass = newMqttPass;

        if (mqttConfigChanged) {
            if (mqttClient.connected()) mqttClient.disconnect();
            mqttClient.setServer(mqttServer.c_str(), mqttPort);
            mqttClient.setCredentials(mqttUser.c_str(), mqttPass.c_str());
            mqttClient.connect();
        }

        String cacheBuster = "/?v=" + String(millis());
        request->redirect(cacheBuster);
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = SPIFFS.open("/update.html", "r");
        if (!file) {
            request->send(500, "text/plain", "Could not open update.html");
            return;
        }
        const String html = file.readString();
        file.close();
        request->send(200, "text/html", html);
    });

    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
                  const bool ok = !Update.hasError();
                  request->send(200, "text/plain", ok ? "Update completed. Restarting..." : "Update failed!");
                  if (ok) {
                      vTaskDelay(pdMS_TO_TICKS(1000));
                      ESP.restart();
                  }
              },
              [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
                  if (!index) {
                      Update.begin();
                  }
                  if (!Update.hasError()) {
                      Update.write(data, len);
                  }
                  if (final) {
                      Update.end(true);
                  }
              });

    server.on("/delete_all", HTTP_GET, [](AsyncWebServerRequest *request) {
        int deleted = 0;
        File root = SPIFFS.open("/events");
        if (!root || !root.isDirectory()) {
            request->send(500, "text/plain", "Could not open /events");
            return;
        }
        File file = root.openNextFile();
        while (file) {
            String fname = file.name();
            if (fname.endsWith(".jpg")) {
                if (!fname.startsWith("/")) {
                    fname = "/events/" + fname;
                }
                if (SPIFFS.exists(fname) && SPIFFS.remove(fname)) {
                    deleted++;
                }
            }
            file = root.openNextFile();
        }
        root.close();
        request->send(200, "text/plain", "Deleted: " + String(deleted) + " images");
        request->redirect("/?v=" + String(millis())); // Cache buster
    });

    server.on("/show_image", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("url")) {
            const String url = request->getParam("url")->value();
            EventBus::instance().publish<API_ShowImageFromUrlEvent>(url);
            request->send(200, "text/plain", "Image will be shown on display!");
        } else {
            request->send(400, "text/plain", "Missing url parameter");
        }
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("[WEB] Reboot requested via /reboot");
        request->send(200, "text/plain", "Rebooting ESP32...");
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP.restart();
    });

    server.on("/keepalive", HTTP_GET, [](AsyncWebServerRequest *request) {
        const auto now = millis();
        const auto until = now + KEEPALIVE_TIMEOUT;
        Serial.println("[WEB] Keep-alive signal received at " + String(now));

        request->send(200, "application/json",
            "{\"now\": " + String(now) + ", \"alive_until\": " + String(until) + "}");
        // Print core
        Serial.println("[WEB] Keep-alive on core " + String(xPortGetCoreID()));
        EventBus::instance().publish<API_KeepAliveEvent>(now);
    });

    server.begin();
}

void WebServer::onMqttConnect(bool sessionPresent) {
    Serial.println("[MQTT] Connected!");
    mqttClient.subscribe(MQTT_TOPIC, 0);
    vTaskSuspend(xTaskGetHandle("WebServer"));
}

void WebServer::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.print("[MQTT] Connection lost with reason: ");
    Serial.println(static_cast<int>(reason));
    EventBus::instance().publish<WEB_MqttDisconnectedEvent>(reason);
    vTaskResume(xTaskGetHandle("WebServer"));
}

void WebServer::onMqttMessage(
    char *topic,
    char *payload,
    AsyncMqttClientMessageProperties properties,
    size_t len,
    size_t index,
    size_t total
) {
    String payloadStr;
    for (size_t i = 0; i < len; i++) payloadStr += (char) payload[i];
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
            zone = String(zonesArray[zonesArray.size() - 1].as<const char *> ());
        }

        if (!detections.isNull() && detections.size() > 0) {
            for (JsonVariant d: detections) {
                auto id = d.as<String>();
                int dashIndex = id.indexOf("-");
                String suffix = (dashIndex > 0) ? id.substring(dashIndex + 1) : id;
                String filename = "/events/" + suffix + "-" + zone + ".jpg";
                EventBus::instance().publish<WEB_ShowLocalImageEvent>(filename);
            }
            String url = "http://" + frigateIp + ":" + String(frigatePort) +
                         "/api/events/" + detections[0].as<String>() + "/snapshot.jpg?crop=1&height=240";
            EventBus::instance().publish<WEB_ShowImageFromUrlWithZoneEvent>(url, zone);
        }
    }
}

String WebServer::getImagesList() {
    String html = "<ul class='image-list'>";
    File root = SPIFFS.open("/events");

    if (!root || !root.isDirectory()) {
        html += "<li>Could not open SPIFFS</li>";
        html += "</ul>";
        return html;
    }

    struct FileInfo {
        String name;
        String displayName;
        unsigned long mtime;
    };
    std::vector<FileInfo> files;

    File file = root.openNextFile();
    while (file) {
        String fname = file.name();
        if (fname.endsWith(".jpg")) {
            String displayName = fname;
            if (!fname.startsWith("/")) {
                fname = "/events/" + fname;
            }
            FileInfo info;
            info.name = fname;
            info.displayName = displayName;
            info.mtime = file.getLastWrite();
            files.push_back(info);
        }
        file = root.openNextFile();
    }
    root.close();

    std::sort(files.begin(), files.end(), [](const FileInfo &a, const FileInfo &b) {
        return a.mtime > b.mtime;
    });

    for (const FileInfo &info: files) {
        html += "<li>";
        html += "<img src='" + info.name + "' alt='Event image'>";
        html += "<a href='" + info.name + "'>" + info.displayName + "</a></li>";
    }

    html += "</ul>";
    return html;
}

String WebServer::getCheckedAttribute(const bool isChecked) {
    return isChecked ? "checked" : "";
}

void WebServer::run(void *pvParameters) {
    for (;;) {
        if (!mqttClient.connected()) {
            mqttClient.setServer(mqttServer.c_str(), mqttPort);
            mqttClient.setCredentials(mqttUser.c_str(), mqttPass.c_str());
            mqttClient.connect();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

