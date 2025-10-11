#include <Arduino.h>
#include <esp_sntp.h>
#include <SPIFFS.h>

#include "Config.h"
#include "Settings.h"
#include "services/DebugService.h"
#include "services/DisplayService.h"
#include "services/NetworkService.h"
#include "services/WeatherService.h"
#include "services/WebService.h"
#include "utils/Diagnostics.h"
#include "utils/Environment.h"

#define ENABLE_DEBUG_SERVICE 0

void heap_caps_alloc_failed_hook(const size_t requested_size, const uint32_t caps, const char *function_name)
{
    DIAG_ENTER_SUPPRESS_IDLE_WDT
    LOG_ERROR("%s was called but failed to allocate %d bytes with 0x%X capabilities", function_name, requested_size, caps);
    Diagnostics::printGlobalHeapWatermark();
    Diagnostics::printHeapUsageSafely();

    if (heap_caps_check_integrity_all(true)) {
        LOG_INFO("Heap integrity check OK");
    }
    else {
        LOG_ERROR("Heap is corrupted");
    }

    DISP_PANIC("Out of memory");
    DIAG_EXIT_SUPPRESS_IDLE_WDT
}

extern "C" void app_main() {
    initArduino();
    Serial.begin(115200);

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set(APP_LOG_TAG, ESP_LOG_VERBOSE);

    heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);

    if (!SPIFFS.begin(true)) {
        LOG_ERROR("Failed to mount SPIFFS! Did you forget to flash the filesystem?");
        // SPIFFS is currently not setup on the Wokwi emulator, so don't panic there
        if (!Environment::isWokwiEmulator()) {
            DISP_PANIC("SPIFFS init failed");
        }
    }

    if (!psramInit()) {
        LOG_ERROR("Failed to initialize PSRAM");
        DISP_PANIC("PSRAM init failed");
    }

    esp_sntp_set_sync_interval(3600 * 1000); // Sync every hour
    // Setup callback for time synchronization
    esp_sntp_set_time_sync_notification_cb([](timeval *) {
        Environment::setTimezone(Settings::instance().timezone);
    });
    configTime(0, 0, SNTP_SERVER);

#if ENABLE_DEBUG_SERVICE == 1
    DebugService::instance().start();
#endif
    DisplayService::instance().start();
    NetworkService::instance().start();
    WeatherService::instance().start();
    WebService::instance().start();
}