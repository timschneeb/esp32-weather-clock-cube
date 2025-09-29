#include <Arduino.h>
#include <esp_sntp.h>
#include <lvgl.h>
#include <SPIFFS.h>
#include <WiFi.h>

// ReSharper disable once CppUnusedIncludeDirective
#include <AsyncHTTPRequest_Generic.h> // Library doesn't handle multiple includes well, needs to be included here

#include "Config.h"
#include "Settings.h"
#include "services/DisplayService.h"
#include "services/NetworkService.h"
#include "services/WeatherService.h"
#include "services/WebService.h"

static void lv_log_print_g_cb(lv_log_level_t level, const char *buf)
{
    Serial.write(buf);
}

void setup() {
    Serial.begin(115200);
    lv_log_register_print_cb(lv_log_print_g_cb);

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS! Did you forget to flash the filesystem?");
        DISP_PANIC("SPIFFS init failed");
    }

    esp_sntp_set_sync_interval(3600 * 1000); // Sync every hour
    // Setup callback for time synchronization
    esp_sntp_set_time_sync_notification_cb([](timeval *) {
        const String timezone = Settings::instance().timezone;
        Serial.printf("Setting Timezone to %s\n", timezone.c_str());
        setenv("TZ", timezone.c_str(), 1);
        tzset();
    });
    configTime(0, 0, SNTP_SERVER);

    DisplayService::instance().start();
    NetworkService::instance().start();
    WeatherService::instance().start();
    WebService::instance().start();
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}