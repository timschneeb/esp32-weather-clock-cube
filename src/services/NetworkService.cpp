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
        if (isConnected() && !isInApMode() &&
            time(nullptr) < 100000 &&
            sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED &&
            sntp_get_sync_status() != SNTP_SYNC_STATUS_IN_PROGRESS) {

            Serial.println("Restarting SNTP time sync...");
            configTime(0, 0, SNTP_SERVER);
        }

        if (isInApMode() || WiFiClass::status() == WL_CONNECTED) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        String ssid = Settings::instance().ssid;
        String pwd = Settings::instance().pwd;

        if (ssid.isEmpty()) {
            Serial.println("No saved SSID, switching to AP mode...");
            enterAPMode();
        } else {
            EventBus::instance().publish<NET_ConnectingEvent>();
            Serial.println("Connecting to WiFi... (SSID: " + ssid + ")");

            WiFiClass::mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), pwd.c_str());
            WiFi.setAutoReconnect(true);

            int retries = 0;
            while (WiFiClass::status() != WL_CONNECTED && retries < 30) {
                vTaskDelay(pdMS_TO_TICKS(500));
                retries++;
            }

            if (WiFiClass::status() != WL_CONNECTED) {
                Serial.println("WiFi failed, fallback to AP mode...");
                enterAPMode();
            } else {
                Serial.println("WiFi connected to: " + Settings::instance().ssid);
                Serial.println("IP address: " + WiFi.localIP().toString());
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
