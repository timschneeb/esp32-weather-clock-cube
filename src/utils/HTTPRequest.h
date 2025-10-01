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
    HTTPRequest() : _request(nullptr), _inProgress(false), _startTime(0), _attempt(0), _retryCount(0), _timeoutMs(10000) {}
    ~HTTPRequest() { if (_request) delete _request; }

    void startRequest(const String& url, const uint32_t retryCount = 3, const uint32_t timeoutMs = 10000) {
        if (_inProgress && _request) {
            LOG_WARN("Previous request still in progress, aborting and deleting AsyncHTTPRequest ptr: %p", _request);
            _request->abort();
            delete _request;
            _request = nullptr;
            _inProgress = false;
        }
        _url = url;
        _timeoutMs = timeoutMs;
        _retryCount = retryCount;
        _attempt = 0;
        _result = {};
        _inProgress = true;
        LOG_DEBUG("Starting HTTP request: %s", _url.c_str());
        initiateRequest();
    }

    bool isInProgress() {
        if (!_inProgress)
            return false;
        if (!_request)
            return false;

        if (!NetworkService::isConnected()) {
            LOG_ERROR("Network disconnected, aborting request... AsyncHTTPRequest ptr: %p", _request);
            _request->abort();
            delete _request;
            _request = nullptr;
            _inProgress = false;
            return false;
        }

        // Timeout check
        if (_timeoutMs != 0 && millis() - _startTime > _timeoutMs) {
            LOG_ERROR("Timeout %llums, aborting AsyncHTTPRequest ptr: %p", millis() - _startTime, _request);
            _request->abort();
            delete _request;
            _request = nullptr;
            if (_attempt < _retryCount) {
                _attempt++;
                LOG_WARN("Retrying HTTP request (attempt %u/%u): %s", _attempt, _retryCount, _url.c_str());
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
        const auto req = new AsyncHTTPRequest();
        req->open("GET", _url.c_str());
        req->onReadyStateChange([](void* arg, AsyncHTTPRequest* cbReq, const int readyState) {
            auto* self = static_cast<HTTPRequest*>(arg);
            if (!self) return;
            // Guard: only handle if this is the current request
            if (self->_request != cbReq) return;
            if (readyState == 4) {
                self->_result.statusCode = cbReq->responseHTTPcode();
                self->_result.success = self->_result.statusCode >= 200 && self->_result.statusCode < 300;
                self->_result.payload = cbReq->responseText();
                LOG_DEBUG("HTTP request finished with status %d: %s (AsyncHTTPRequest ptr: %p)", self->_result.statusCode, self->_url.c_str(), cbReq);
                self->_inProgress = false;
                cbReq->abort();
                self->_request = nullptr;
                // DO NOT delete cbReq here!
            }
            else if (readyState < 0) {
                LOG_ERROR("HTTP request error (state=%d) %s (AsyncHTTPRequest ptr: %p)", readyState, self->_url.c_str(), cbReq);
                cbReq->abort();
                self->_request = nullptr;
                self->_inProgress = false;
                // DO NOT delete cbReq here!
            }
        }, this);
        _request = req;
        req->send();
    }

    AsyncHTTPRequest* _request;
    String _url;
    bool _inProgress;
    uint64_t _startTime;
    uint32_t _attempt;
    uint32_t _retryCount;
    uint32_t _timeoutMs;
    HTTPResult _result;
};


#endif //ASYNCHTTPREQUESTTASK_H