#include <Arduino.h>
#include <esp_sntp.h>
#include <SPIFFS.h>

// ReSharper disable once CppUnusedIncludeDirective
#include <AsyncHTTPRequest_Generic.h> // Library doesn't handle multiple includes well, needs to be included here

#include "Config.h"
#include "Settings.h"
#include "services/DebugService.h"
#include "services/DisplayService.h"
#include "services/NetworkService.h"
#include "services/WeatherService.h"
#include "services/WebService.h"

void heap_caps_alloc_failed_hook(const size_t requested_size, const uint32_t caps, const char *function_name)
{
    LOG_ERROR("%s was called but failed to allocate %d bytes with 0x%X capabilities", function_name, requested_size, caps);
}

void setup() {
    Serial.begin(115200);
    heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);

    if (!SPIFFS.begin(true)) {
        LOG_ERROR("Failed to mount SPIFFS! Did you forget to flash the filesystem?");
        DISP_PANIC("SPIFFS init failed");
    }

    if (!psramInit()) {
        LOG_ERROR("Failed to initialize PSRAM");
        DISP_PANIC("PSRAM init failed");
    }

    esp_sntp_set_sync_interval(3600 * 1000); // Sync every hour
    // Setup callback for time synchronization
    esp_sntp_set_time_sync_notification_cb([](timeval *) {
        const String timezone = Settings::instance().timezone;
        LOG_INFO("Setting Timezone to %s\n", timezone.c_str());
        setenv("TZ", timezone.c_str(), 1);
        tzset();
    });
    configTime(0, 0, SNTP_SERVER);

    DebugService::instance().start();
    DisplayService::instance().start();
    NetworkService::instance().start();
    WeatherService::instance().start();
    WebService::instance().start();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}