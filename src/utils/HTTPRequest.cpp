#include "HTTPRequest.h"

#include <esp_http_client.h>

#include "services/NetworkService.h"

static std::atomic<uint32_t> requestCount = 0;

HTTPRequest::HTTPRequest() {}
HTTPRequest::~HTTPRequest() { cleanupClient(); }

void HTTPRequest::startRequest(const String& url, const uint32_t retryCount, const uint32_t timeoutMs) {
    cleanupClient();
    _url = url;
    _retryCount = retryCount;
    _timeoutMs = timeoutMs;
    _attempt = 1;
    _result = {};
    _responseBuffer = "";
    _inProgress = true;
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }

    xTaskCreatePinnedToCore(requestTask, String("HTTP@" + String(requestCount++)).c_str(), 4096, this, 1, &_taskHandle, 1);
}

void HTTPRequest::requestTask(void* pvParameters) {
    auto* self = static_cast<HTTPRequest*>(pvParameters);
    self->runRequest();
    self->_inProgress = false;
    self->_taskHandle = nullptr;
    vTaskDelete(nullptr);
}

void HTTPRequest::runRequest() {
    String encodedUrl = encodeUrl(_url);

    for (_attempt = 1; _attempt <= _retryCount; ++_attempt) {
        if (!NetworkService::isConnected()) {
            LOG_ERROR("Network disconnected, aborting request...");
            _result.success = false;
            _result.statusCode = -1;
            _result.payload = "";
            return;
        }
        esp_http_client_config_t config = {};
        config.url = encodedUrl.c_str();
        config.timeout_ms = _timeoutMs;
        config.method = HTTP_METHOD_GET;
        config.buffer_size = 512;
        config.buffer_size_tx = 512;
        _client = esp_http_client_init(&config);
        if (!_client) {
            LOG_ERROR("Failed to init esp_http_client");
            _result.success = false;
            _result.statusCode = -2;
            _result.payload = "";
            return;
        }
        const esp_err_t err = esp_http_client_perform(_client);
        const int status = esp_http_client_get_status_code(_client);
        const int content_length = esp_http_client_get_content_length(_client);
        if (err == ESP_OK && status >= 200 && status < 300) {
            String payload;
            payload.reserve(content_length > 0 ? content_length : 512);
            char buffer[256];
            int read_len = 0;
            esp_http_client_set_url(_client, encodedUrl.c_str());
            esp_http_client_open(_client, 0);
            while ((read_len = esp_http_client_read(_client, buffer, sizeof(buffer)-1)) > 0) {
                buffer[read_len] = 0;
                payload += buffer;
            }
            _result.success = true;
            _result.statusCode = status;
            _result.payload = payload;
            LOG_DEBUG("HTTP request finished with status %d: %s", status, _url.c_str());
            esp_http_client_cleanup(_client);
            _client = nullptr;
            return;
        }

        LOG_ERROR("HTTP request failed (err=%d, status=%d), attempt %u/%u: %s", err, status, _attempt, _retryCount, _url.c_str());
        esp_http_client_cleanup(_client);
        _client = nullptr;
        _result.success = false;
        _result.statusCode = status;
        _result.payload = "";
        vTaskDelay(pdMS_TO_TICKS(100)); // Small delay before retry
    }
}

void HTTPRequest::cleanupClient() {
    if (_client) {
        esp_http_client_cleanup(_client);
        _client = nullptr;
    }
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
}

String HTTPRequest::encodeUrl(const String &url) {
    // Find scheme
    int scheme_end = url.indexOf("://");
    String scheme = "";
    int host_start = 0;
    if (scheme_end >= 0) {
        scheme = url.substring(0, scheme_end + 3); // include '://'
        host_start = scheme_end + 3;
    }
    // Find path start
    int path_start = url.indexOf('/', host_start);
    String host_port = path_start >= 0 ? url.substring(host_start, path_start) : url.substring(host_start);
    // Find query start
    int query_start = url.indexOf('?', path_start >= 0 ? path_start : host_start);
    String path = "";
    String query = "";
    if (path_start >= 0) {
        if (query_start >= 0) {
            path = url.substring(path_start, query_start);
            query = url.substring(query_start + 1);
        } else {
            path = url.substring(path_start);
        }
    }
    // Encode path segments
    String encodedPath = "";
    int seg_start = 0;
    for (int i = 0; i <= path.length(); ++i) {
        if (i == path.length() || path[i] == '/') {
            String segment = path.substring(seg_start, i);
            if (segment.length() > 0) {
                encodedPath += encodeUrlSegment(segment);
            }
            if (i < path.length()) encodedPath += '/';
            seg_start = i + 1;
        }
    }
    // Encode query parameters
    String encodedQuery = "";
    if (query.length() > 0) {
        int param_start = 0;
        for (int i = 0; i <= query.length(); ++i) {
            if (i == query.length() || query[i] == '&') {
                String param = query.substring(param_start, i);
                int eq_pos = param.indexOf('=');
                if (eq_pos >= 0) {
                    encodedQuery += encodeUrlSegment(param.substring(0, eq_pos));
                    encodedQuery += "=";
                    encodedQuery += encodeUrlSegment(param.substring(eq_pos + 1));
                } else {
                    encodedQuery += encodeUrlSegment(param);
                }
                if (i < query.length()) encodedQuery += "&";
                param_start = i + 1;
            }
        }
    }
    // Rebuild URL
    String result = scheme + host_port;
    if (encodedPath.length() > 0) result += encodedPath;
    if (encodedQuery.length() > 0) result += "?" + encodedQuery;
    return result;
}

String HTTPRequest::encodeUrlSegment(const String &value) {
    String escaped = "";
    for (const char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped += c;
        } else {
            const auto hex = "0123456789ABCDEF";
            escaped += '%';
            escaped += hex[(c >> 4) & 0xF];
            escaped += hex[c & 0xF];
        }
    }
    return escaped;
}
