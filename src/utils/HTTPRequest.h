//
// Created by tim on 25.09.25.
//

#ifndef ASYNCHTTPREQUESTTASK_H
#define ASYNCHTTPREQUESTTASK_H

#include <Arduino.h>
#include <AsyncHTTPRequest_Generic.hpp>

#include "services/NetworkService.h"

struct HTTPResult {
    String payload;        // Response body
    int statusCode = 0;    // HTTP status code
    bool success = false;  // True if >=200 and <300
};

class HTTPRequest {
public:
    HTTPRequest() : _inProgress(false), _attempt(0), _retryCount(0), _timeoutMs(10000), _startTime(0) {}

    void startRequest(const String& url, const uint32_t retryCount = 3, const uint32_t timeoutMs = 10000) {
        _url = url;
        _timeoutMs = timeoutMs;
        _retryCount = retryCount;
        _attempt = 0;
        _result = {};
        _inProgress = true;
        initiateRequest();
    }

    bool isInProgress() {
        if (!_inProgress)
            return false;

        if (!NetworkService::isConnected()) {
            LOG_ERROR("Network disconnected, aborting request...");
            request.abort();
            return false;
        }

        // Timeout check
        if (_timeoutMs != 0 && millis() - _startTime > _timeoutMs) {
            LOG_ERROR("Timeout %llms, retrying...", millis() - _startTime);
            if (_attempt < _retryCount) {
                _attempt++;
                request.abort();
                initiateRequest();
            } else {
                _inProgress = false;
            }
        }

        return _inProgress;
    }

    HTTPResult result() const { return _result; }

private:
    void initiateRequest() {
        _result = {};
        _startTime = millis();

        request.open("GET", _url.c_str());
        request.onReadyStateChange([this](void*, AsyncHTTPRequest* req, const int readyState) {
            if (readyState == 4) {
                _result.statusCode = req->responseHTTPcode();
                _result.success = _result.statusCode >= 200 && _result.statusCode < 300;
                _result.payload = req->responseText();
                LOG_DEBUG("HTTP request finished with status %d: %s", _result.statusCode, _url.c_str());

                // Only mark finished if success or retries exhausted
                if (!_result.success && _attempt < _retryCount) {
                    _attempt++;
                    request.abort();
                    initiateRequest();
                } else {
                    _inProgress = false;
                }
            }
            else if (readyState < 0) {
                LOG_ERROR("HTTP request error (state=%d) %s", readyState, _url.c_str());
                if (_attempt < _retryCount) {
                    _attempt++;
                    request.abort();
                    initiateRequest();
                } else {
                    _inProgress = false;
                }
            }
        });
        request.send();
    }

    AsyncHTTPRequest request;
    String _url;
    bool _inProgress;
    uint64_t _startTime;
    uint32_t _attempt;
    uint32_t _retryCount;
    uint32_t _timeoutMs;
    HTTPResult _result;
};


#endif //ASYNCHTTPREQUESTTASK_H
