//
// Created by tim on 24.09.25.
//

#ifndef WEBAPI_H
#define WEBAPI_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

typedef std::function<void(const String& server, int port, const String& user, const String& pass)> OnMqttConfigChangedCb;

class WebApi final {
public:
    WebApi();

    void setOnMqttConfigChanged(const OnMqttConfigChangedCb &callback);

private:
    AsyncWebServer server;
    Preferences preferences;
    OnMqttConfigChangedCb onMqttConfigChanged;

    void onRootRequest(AsyncWebServerRequest *request);
    void onSaveRequest(AsyncWebServerRequest *request);
    void onUpdateRequest(AsyncWebServerRequest *request);
    void onUpdatePostRequest(AsyncWebServerRequest *request);
    void onUpdatePostUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
    void onDeleteAllRequest(AsyncWebServerRequest *request);
    void onShowImageRequest(AsyncWebServerRequest *request);
    void onRebootRequest(AsyncWebServerRequest *request);
    void onKeepAliveRequest(AsyncWebServerRequest *request);

    void onDiagRequest(AsyncWebServerRequest *request);

    static String getImagesList();
    static String getCheckedAttribute(bool isChecked);
};

#endif // WEBAPI_H
