//
// Created by tim on 24.09.25.
//

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

class AsyncWebServer;
class AsyncWebServerRequest;

#include <WString.h>
#include <map>

#include "Task.h"
#include "utils/Macros.h"

class WebService final : public Task {
    SINGLETON(WebService)
public:
    ~WebService() override;

protected:
    [[noreturn]] void run() override;

private:
    AsyncWebServer* server;

    struct UploadState {
        uint8_t* buffer = nullptr;
        size_t size = 0;
        size_t offset = 0;
        uint64_t lastActivityTime = 0;
    };
    std::map<String, UploadState> uploadStates;

    void onRootRequest(AsyncWebServerRequest *request);
    void onSaveRequest(AsyncWebServerRequest *request) const;
    void onUpdateRequest(AsyncWebServerRequest *request);
    void onUpdatePostRequest(AsyncWebServerRequest *request);
    void onUpdatePostUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
    void onDeleteAllRequest(AsyncWebServerRequest *request);
    void onShowImageRequest(AsyncWebServerRequest *request);
    void onShowImagePostRequest(AsyncWebServerRequest *request);

    void printImageUploadStates();

    void onShowImagePostUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,size_t len, bool final);
    void onShowMessageRequest(AsyncWebServerRequest *request);
    void onRebootRequest(AsyncWebServerRequest *request);
    void onKeepAliveRequest(AsyncWebServerRequest *request);
    void onDiagRequest(AsyncWebServerRequest *request);

    static String getImagesList();
    static String getCheckedAttribute(bool isChecked);
};

#endif // WEBINTERFACE_H
