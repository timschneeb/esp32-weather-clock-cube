//
// Created by tim on 25.09.25.
//

#ifndef ASYNCHTTPREQUESTTASK_H
#define ASYNCHTTPREQUESTTASK_H

#include <Arduino.h>

// Forward declaration esp_http_client.h
struct esp_http_client;
typedef esp_http_client *esp_http_client_handle_t;

struct HTTPResult {
    String payload;        // Response body
    int statusCode = 0;    // HTTP status code
    bool success = false;  // True if >=200 and <300
};

class HTTPRequest {
public:
    HTTPRequest();
    ~HTTPRequest();

    void startRequest(const String& url, uint32_t retryCount = 3, uint32_t timeoutMs = 10000);
    bool isInProgress() const { return _inProgress; }
    HTTPResult result() const { return _result; }

private:
    static String encodeUrl(const String &value);
    static void requestTask(void* pvParameters);
    void runRequest();
    void cleanupClient();

    String _url;
    bool _inProgress = false;
    uint32_t _attempt = 0;
    uint32_t _retryCount = 0;
    uint32_t _timeoutMs = 10000;
    HTTPResult _result;
    esp_http_client_handle_t _client = nullptr;
    String _responseBuffer;
    TaskHandle_t _taskHandle = nullptr;
};

#endif //ASYNCHTTPREQUESTTASK_H
