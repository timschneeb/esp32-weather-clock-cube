//
// Created by tim on 24.09.25.
//

#include "NetworkService.h"

#include <esp_sntp.h>
#include <WiFi.h>

#include "Config.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "Settings.h"
#include "utils/Environment.h"
#include "utils/Macros.h"

constexpr auto WIFI_TIMEOUT = 25000;

NetworkService::NetworkService() : Task("NetworkService", 4096, 1) {}

String NetworkService::getSavedSSID() {
    return Settings::instance().ssid;
}

bool NetworkService::isConnected() {
    return WiFiClass::status() == WL_CONNECTED;
}

[[noreturn]] void NetworkService::run() {
    for (;;) {
        // If initial time sync hasn't completed yet, retry
        if (isConnected() && !isInApMode() && !Environment::isTimeSynchronized()) {
            LOG_DEBUG("Restarting SNTP time sync...");
            configTime(0, 0, SNTP_SERVER);
        }

        if (isInApMode() || WiFiClass::status() == WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        String ssid = Settings::instance().ssid;
        String pwd = Settings::instance().pwd;

        if (Environment::isWokwiEmulator()) {
            LOG_DEBUG("Using virtual Wokwi emulator network");
            ssid = "Wokwi-GUEST";
            pwd = "";
        }

        if (ssid.isEmpty()) {
            LOG_DEBUG("No saved SSID, switching to AP mode...");
            enterAPMode();
        } else {
            EventBus::instance().publish<NET_ConnectingEvent>();
            LOG_DEBUG("Connecting to WiFi... (SSID: %s)", ssid.c_str());

            // TODO: fix unreliable connection on startup
            WiFiClass::mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), pwd.c_str());
            WiFi.setAutoReconnect(true);

            int retries = 0;
            while (WiFiClass::status() != WL_CONNECTED && retries < 60) {
                vTaskDelay(pdMS_TO_TICKS(2000));
                retries++;
            }

            if (WiFiClass::status() != WL_CONNECTED) {
                LOG_ERROR("WiFi failed, fallback to AP mode...");
                enterAPMode();
            } else {
                LOG_DEBUG("WiFi connected to: %s", Settings::instance().ssid.load().c_str());
                LOG_DEBUG("IP address: %s", WiFi.localIP().toString().c_str());
                EventBus::instance().publish<NET_StaConnectedEvent>();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool NetworkService::isInApMode() {
    return WiFiClass::getMode() == WIFI_MODE_AP;
}

void NetworkService::enterAPMode() {
    WiFiClass::mode(WIFI_AP);
    WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
    EventBus::instance().publish<NET_ApCreatedEvent>();
}
