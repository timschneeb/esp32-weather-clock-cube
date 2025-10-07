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
#include "utils/Environment.h"
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
    server.on("/show_image", HTTP_POST, BIND_CB(onShowImagePostRequest), BIND_UPLOAD_CB(onShowImagePostUpload));
    server.on("/show_message", HTTP_GET, BIND_CB(onShowMessageRequest));
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
    const auto config = &Settings::instance();

    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        request->send(500, "text/plain", "Could not open index.html\n");
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
    html.replace("{{sec}}", String(config->displayDuration));
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

    settings->displayDuration = getIntParam(request, "sec", 30, 1);

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
        Environment::setTimezone(settings->timezone);
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

    LOG_DEBUG("Settings saved");
    Settings::instance().save();

    String cacheBuster = "/?v=" + String(millis());
    request->redirect(cacheBuster);
}

void WebApi::onUpdateRequest(AsyncWebServerRequest *request) {
    File file = SPIFFS.open("/update.html", "r");
    if (!file) {
        request->send(500, "text/plain", "Could not open update.html\n");
        return;
    }
    const String html = file.readString();
    file.close();
    request->send(200, "text/html", html);
}

void WebApi::onUpdatePostRequest(AsyncWebServerRequest *request) {
    const bool ok = !Update.hasError();
    request->send(200, "text/plain", ok ? "Update completed. Restarting...\n" : "Update failed!\n");
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
        request->send(500, "text/plain", "Could not open /events\n");
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
    request->send(200, "text/plain", "Deleted: " + String(deleted) + " images\n");
    request->redirect("/?v=" + String(millis())); // Cache buster
}

void WebApi::onShowImageRequest(AsyncWebServerRequest *request) {
    if (request->hasParam("url")) {
        const String url = request->getParam("url")->value();
        EventBus::instance().publish<API_ShowImageFromUrlEvent>(url);
        request->send(200, "text/plain", "Image will be shown on display!\n");
    } else {
        request->send(400, "text/plain", "Missing url parameter\n");
    }
}

void WebApi::onShowImagePostRequest(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Uploaded\n");
}

String findFreeFileSlot() {
    for (int i = 1; i <= 3; i++) {
        String path = "/events/uploaded_" + String(i) + ".jpg";
        if (!SPIFFS.exists(path)) {
            return path;
        }
    }

    LOG_WARN("No free upload slots, overwriting uploaded_1.jpg");
    SPIFFS.remove("/events/uploaded_1.jpg");
    return "/events/uploaded_1.jpg";
}

void WebApi::onShowImagePostUpload(AsyncWebServerRequest *request, const String &filename, const size_t index,
                                   uint8_t *data, const size_t len, const bool final) {
    constexpr size_t maxUploadSize = 2 * 1024 * 1024; // 2MB max per image

    if (index == 0U) {
        // Start of upload: allocate buffer in PSRAM
        uploadStates[filename] = {
            .buffer = static_cast<uint8_t *>(heap_caps_malloc(maxUploadSize, MALLOC_CAP_SPIRAM)),
            .size = maxUploadSize,
            .offset = 0,
            .lastActivityTime = millis()
        };

        if (!uploadStates[filename].buffer) {
            request->send(500, "text/plain", "PSRAM allocation failed\n");
            uploadStates.erase(filename);
            return;
        }
    }
    else if (uploadStates.find(filename) == uploadStates.end()) {
        // No state found for this ongoing upload. Likely expired
        request->send(408, "text/plain", "Session expired\n");
        return;
    }

    auto &state = uploadStates[filename];
    if (state.buffer && (state.offset + len <= state.size)) {
        memcpy(state.buffer + state.offset, data, len);
        state.offset += len;
        if (final) {
            // Save buffer to SPIFFS as image file
            String savePath = findFreeFileSlot();
            File imgFile = SPIFFS.open(savePath, "w");
            if (!imgFile) {
                request->send(500, "text/plain", "Failed to open file for writing: " + savePath + "\n");
                heap_caps_free(state.buffer);
                uploadStates.erase(filename);
                return;
            }
            const size_t written = imgFile.write(state.buffer, state.offset);
            imgFile.close();
            heap_caps_free(state.buffer);
            uploadStates.erase(filename);
            if (written != state.offset) {
                request->send(500, "text/plain", "Failed to write complete image. Written: " + String(written) + "\n");
            } else {
                EventBus::instance().publish<API_ShowImageEvent>(savePath);
            }
        }
    } else if (state.buffer) {
        // Buffer overflow
        heap_caps_free(state.buffer);
        uploadStates.erase(filename);
        request->send(413, "text/plain", "Upload too large (max " + String(maxUploadSize) + " bytes)\n");
    }
}

void WebApi::onShowMessageRequest(AsyncWebServerRequest *request) {
    if (request->hasParam("msg")) {
        const String msg = request->getParam("msg")->value();
        const uint32_t duration = request->hasParam("duration") ? request->getParam("duration")->value().toInt() : 5000;
        EventBus::instance().publish<API_ShowMessageEvent>(msg, duration);
        request->send(200, "text/plain", "Message will be shown on display!\n");
    } else {
        request->send(400, "text/plain", "Missing msg parameter\n");
    }
}

void WebApi::onRebootRequest(AsyncWebServerRequest *request) {
    LOG_INFO("Reboot requested");
    request->send(200, "text/plain", "Rebooting ESP32...\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP.restart();
}

void WebApi::onKeepAliveRequest(AsyncWebServerRequest *request) {
    const auto now = millis();
    const auto until = now + KEEPALIVE_TIMEOUT;
    LOG_DEBUG("Signal received at %lu", now);
    request->send(200, "application/json", "{\"now\": " + String(now) + ", \"alive_until\": " + String(until) + "}\n");
    EventBus::instance().publish<API_KeepAliveEvent>(now);
}

void WebApi::onDiagRequest(AsyncWebServerRequest *request) {
    const auto heap = Diagnostics::collectHeapUsage();
    request->send(200, "text/plain", heap + "\n");
    LOG_INFO("%s", heap.c_str());
    Diagnostics::printGlobalHeapWatermark();
}

void WebApi::tick() {
    // Clean up any abandoned uploads (e.g. client disconnected)
    for (auto it = uploadStates.begin(); it != uploadStates.end(); ) {
        if (millis() - it->second.lastActivityTime < 30000) {
            ++it;
            continue;
        }

        LOG_DEBUG("Abandoned upload for %s, cleaning up", it->first.c_str());
        if (it->second.buffer) {
            heap_caps_free(it->second.buffer);
        }
        it = uploadStates.erase(it);
    }
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
