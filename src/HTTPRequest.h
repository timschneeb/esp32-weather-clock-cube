//
// Created by tim on 25.09.25.
//

#ifndef ASYNCHTTPREQUESTTASK_H
#define ASYNCHTTPREQUESTTASK_H

#include <Arduino.h>
#include <AsyncHTTPRequest_Generic.hpp>

struct HTTPResult {
    String payload;        // Response body
    int statusCode;        // HTTP status code
    bool success;          // True if >=200 and <300
};

class HTTPRequest {
public:
    HTTPRequest() : inProgress(false), startTime(0), attempt(0), retryCount(0), timeoutMs(10000), _result() {}

    void startRequest(const String& url, const uint32_t retryCount = 3, const uint32_t timeoutMs = 10000) {
        this->url = url;
        this->timeoutMs = timeoutMs;
        this->retryCount = retryCount;
        attempt = 0;
        _result = {};
        inProgress = true;
        initiateRequest();
    }

    bool isInProgress() {
        if (!inProgress)
            return false;

        // Timeout check
        if (millis() - startTime > timeoutMs) {
            Serial.println("[HTTP] Timeout, retrying...");
            if (attempt < retryCount) {
                attempt++;
                request.abort();
                initiateRequest();
            } else {
                inProgress = false;
            }
        }

        return inProgress;
    }

    HTTPResult result() const { return _result; }

private:
    void initiateRequest() {
        _result = {};
        startTime = millis();

        request.open("GET", url.c_str());
        request.onReadyStateChange([this](void*, AsyncHTTPRequest* req, const int readyState) {
            if (readyState == 4) {
                _result.statusCode = req->responseHTTPcode();
                _result.success = _result.statusCode >= 200 && _result.statusCode < 300;
                _result.payload = req->responseText();

                // Only mark finished if success or retries exhausted
                if (!_result.success && attempt < retryCount) {
                    attempt++;
                    initiateRequest();
                } else {
                    inProgress = false;
                }
            }
        });
        request.send();
    }

    AsyncHTTPRequest request;
    String url;
    bool inProgress;
    uint64_t startTime;
    uint32_t attempt;
    uint32_t retryCount;
    uint32_t timeoutMs;
    HTTPResult _result;
};


#endif //ASYNCHTTPREQUESTTASK_H
