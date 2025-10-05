//
// Created by tim on 24.09.25.
//

// ReSharper disable CppMemberFunctionMayBeStatic
#include "WebApi.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Update.h>

#include <algorithm>

#include "Config.h"
#include "services/NetworkService.h"
#include "Settings.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "utils/Diagnostics.h"
#include "utils/Macros.h"

using namespace ArduinoJson;

WebApi::WebApi() : server(80) {
    server.serveStatic("/styles.css", SPIFFS, "/styles.css");
    server.serveStatic("/scripts.js", SPIFFS, "/scripts.js");
    server.serveStatic("/events", SPIFFS, "/events");
    server.serveStatic("/icons", SPIFFS, "/icons");

#define BIND_CB(method) std::bind(&WebApi::method, this, std::placeholders::_1)
#define BIND_UPLOAD_CB(method) std::bind(&WebApi::method, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6)
    server.on("/", HTTP_GET, BIND_CB(onRootRequest));
    server.on("/save", HTTP_POST, BIND_CB(onSaveRequest));
    server.on("/update", HTTP_GET, BIND_CB(onUpdateRequest));
    server.on("/update", HTTP_POST, BIND_CB(onUpdatePostRequest), BIND_UPLOAD_CB(onUpdatePostUpload));
    server.on("/delete_all", HTTP_GET, BIND_CB(onDeleteAllRequest));
    server.on("/show_image", HTTP_GET, BIND_CB(onShowImageRequest));
    server.on("/reboot", HTTP_GET, BIND_CB(onRebootRequest));
    server.on("/keepalive", HTTP_GET, BIND_CB(onKeepAliveRequest));
    server.on("/diag", HTTP_GET, BIND_CB(onDiagRequest));
#undef BIND_UPLOAD_CB
#undef BIND_CB

    server.begin();
}

void WebApi::setOnMqttConfigChanged(const OnMqttConfigChangedCb &callback) {
    onMqttConfigChanged = callback;
}

void WebApi::onRootRequest(AsyncWebServerRequest *request) {
    auto *const config = &Settings::instance();
    config->load();
    String mode = config->mode;
    mode.toLowerCase();
    const bool isAlertChecked = mode.indexOf("alert") >= 0;
    const bool isDetectionChecked = mode.indexOf("detection") >= 0;

    const String alertCheckbox = "<input type='checkbox' id='mode_alert' name='mode_alert' value='alert' " +
                                 getCheckedAttribute(isAlertChecked) + ">";
    const String detectionCheckbox =
            "<input type='checkbox' id='mode_detection' name='mode_detection' value='detection' "
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
    html.replace("{{slideshowInterval}}", String(config->slideshowSec));
    html.replace("{{alertCheckbox}}", alertCheckbox);
    html.replace("{{detectionCheckbox}}", detectionCheckbox);
    html.replace("{{weatherApiKey}}", config->weatherApiKey.load() != "" ? "******" : "");
    html.replace("{{weatherApiKey_exists}}", config->weatherApiKey.load() != "" ? "1" : "0");
    html.replace("{{weatherCity}}", config->weatherCity);
    html.replace("{{timezoneName}}", String(config->timezone));
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
}

void WebApi::onSaveRequest(AsyncWebServerRequest *request) const {
    // Safe integer fetch helper
    // Enforces min/max ranges and uses default if out of range or not present
    auto getIntParam = [](const AsyncWebServerRequest *req, const char *name,
                          const int def, const int min = INT32_MIN, const int max = INT32_MAX) -> int {
        if (req->getParam(name, true)) {
            const String v = req->getParam(name, true)->value();
            if (v.length() > 0) {
                const int value = v.toInt();
                if (value >= min && value <= max)
                    return value;
            }
        }
        return def;
    };

    auto *settings = &Settings::instance();

    auto oldMqttServer = settings->mqtt.load();
    auto oldMqttPort = settings->mqttPort.load();
    auto oldMqttUser = settings->mqttUser.load();
    auto oldMqttPass = settings->mqttPass.load();

    settings->ssid = request->getParam("ssid", true)->value();

    String newPwd = request->getParam("pwd", true)->value();
    bool pwdExists = request->getParam("pwd_exists", true)->value() == "1";
    if (!pwdExists || newPwd != "******") {
        settings->pwd = newPwd;
    }

    settings->fip = request->getParam("fip", true)->value();
    settings->fport = getIntParam(request, "fport", 5000, 1, 65535);

    settings->displayDuration = getIntParam(request, "sec", 30, 1);
    settings->maxImages = getIntParam(request, "maxImages", 0, 1, 60);
    settings->slideshowSec = getIntParam(request, "slideInterval", 0, 500, 20000);

    // Modes
    String modeValue = "";
    if (request->hasParam("mode_alert", true)) {
        if (request->getParam("mode_alert", true)->value() == "alert") {
            modeValue += "alert";
        }
    }
    if (request->hasParam("mode_detection", true)) {
        if (request->getParam("mode_detection", true)->value() == "detection") {
            if (modeValue != "")
                modeValue += ",";
            modeValue += "detection";
        }
    }
    settings->mode = modeValue;

    // Weather
    String newApiKey = request->getParam("weatherApiKey", true)->value();
    bool apiKeyExists = request->getParam("weatherApiKey_exists", true)->value() == "1";
    if (!apiKeyExists || newApiKey != "******") {
        settings->weatherApiKey = newApiKey;
    }

    settings->weatherCity = request->getParam("weatherCity", true)->value();

    auto oldTimezone = settings->timezone.load();
    auto oldBrightness = settings->brightness.load();
    settings->timezone = request->getParam("timezoneName", true)->value();
    settings->brightness = getIntParam(request, "brightness", 0, 0, 255);

    if (oldTimezone != settings->timezone.load()) {
        // Timezone changed, update system timezone
        // TODO: Duplicated, create util class
        LOG_INFO("Setting Timezone to %s\n", settings->timezone.load().c_str());
        setenv("TZ", settings->timezone.load().c_str(), 1);
        tzset();
    }

    if (oldBrightness != settings->brightness.load()) {
        EventBus::instance().publish<CFG_BrightnessUpdatedEvent>(settings->brightness.load());
    }

    // MQTT
    String newMqtt, newMqttUser, newMqttPass;
    int newMqttPort;
    settings->mqtt = newMqtt = request->getParam("mqtt", true)->value();
    settings->mqttPort = newMqttPort = getIntParam(request, "mqttport", 1883, 1, 65535);
    settings->mqttUser = newMqttUser = request->getParam("mqttuser", true)->value();
    newMqttPass = request->getParam("mqttpass", true)->value();

    bool mqttPassExists = request->getParam("mqttpass_exists", true)->value() == "1";
    if (!mqttPassExists || newMqttPass != "******") {
        settings->mqttPass = newMqttPass;
    }

    const bool mqttConfigChanged = newMqtt != oldMqttServer ||
                                   newMqttPort != oldMqttPort ||
                                   newMqttUser != oldMqttUser ||
                                   (newMqttPass != "******" && newMqttPass != oldMqttPass);
    if (mqttConfigChanged) {
        onMqttConfigChanged(newMqtt, newMqttPort, newMqttUser, newMqttPass != "******" ? newMqttPass : oldMqttPass);
    }

    Settings::instance().save();

    String cacheBuster = "/?v=" + String(millis());
    request->redirect(cacheBuster);
}

void WebApi::onUpdateRequest(AsyncWebServerRequest *request) {
    File file = SPIFFS.open("/update.html", "r");
    if (!file) {
        request->send(500, "text/plain", "Could not open update.html");
        return;
    }
    const String html = file.readString();
    file.close();
    request->send(200, "text/html", html);
}

void WebApi::onUpdatePostRequest(AsyncWebServerRequest *request) {
    const bool ok = !Update.hasError();
    request->send(200, "text/plain", ok ? "Update completed. Restarting..." : "Update failed!");
    if (ok) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP.restart();
    }
}

void WebApi::onUpdatePostUpload(AsyncWebServerRequest *request, const String &filename, const size_t index,
                                uint8_t *data,
                                const size_t len, const bool final) {
    if (index == 0U) {
        Update.begin();
    }
    if (!Update.hasError()) {
        Update.write(data, len);
    }
    if (final) {
        Update.end(true);
    }
}

void WebApi::onDeleteAllRequest(AsyncWebServerRequest *request) {
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
}

void WebApi::onShowImageRequest(AsyncWebServerRequest *request) {
    if (request->hasParam("url")) {
        const String url = request->getParam("url")->value();
        EventBus::instance().publish<API_ShowImageFromUrlEvent>(url);
        request->send(200, "text/plain", "Image will be shown on display!");
    } else {
        request->send(400, "text/plain", "Missing url parameter");
    }
}

void WebApi::onRebootRequest(AsyncWebServerRequest *request) {
    LOG_INFO("Reboot requested");
    request->send(200, "text/plain", "Rebooting ESP32...");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP.restart();
}

void WebApi::onKeepAliveRequest(AsyncWebServerRequest *request) {
    const auto now = millis();
    const auto until = now + KEEPALIVE_TIMEOUT;
    LOG_DEBUG("Signal received at %lu", now);
    request->send(200, "application/json", "{\"now\": " + String(now) + ", \"alive_until\": " + String(until) + "}");
    EventBus::instance().publish<API_KeepAliveEvent>(now);
}

void WebApi::onDiagRequest(AsyncWebServerRequest *request) {
    const auto heap = Diagnostics::collectHeapUsage();
    request->send(200, "text/plain", heap);
    LOG_INFO("%s", heap.c_str());
    Diagnostics::printGlobalHeapWatermark();
}

String WebApi::getImagesList() {
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
        unsigned long mtime = 0;
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

String WebApi::getCheckedAttribute(const bool isChecked) {
    return isChecked ? "checked" : "";
}
