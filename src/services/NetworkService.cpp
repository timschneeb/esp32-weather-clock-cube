//
// Created by tim on 24.09.25.
//

#include "NetworkService.h"

#include <esp_sntp.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <string>
#include <lwip/ip4_addr.h>

#include "Config.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "Settings.h"
#include "utils/Environment.h"
#include "utils/Macros.h"

constexpr auto WIFI_TIMEOUT = 25000;
constexpr int WIFI_CONNECT_MAX_ATTEMPTS = 15;
bool NetworkService::s_isConnected = false;
bool NetworkService::s_isInApMode = false;

NetworkService::NetworkService() : Task("NetworkService", 4096, 1) {}

std::string NetworkService::getSavedSSID() {
    return std::string(Settings::instance().ssid.load().c_str());
}

bool NetworkService::isConnected() {
    return s_isConnected;
}

bool NetworkService::isInApMode() {
    return s_isInApMode;
}

void NetworkService::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_CONNECTED:
                LOG_DEBUG("WiFi STA connected");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                LOG_ERROR("WiFi STA disconnected, retrying...");
                s_isConnected = false;
                esp_wifi_connect();
                break;
            case WIFI_EVENT_AP_START:
                LOG_DEBUG("WiFi AP started");
                s_isInApMode = true;
                break;
            case WIFI_EVENT_AP_STOP:
                LOG_DEBUG("WiFi AP stopped");
                s_isInApMode = false;
                break;
            default: ;
        }
    }
}

void NetworkService::ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_isConnected = true;
        s_isInApMode = false;
        LOG_DEBUG("WiFi connected, got IP");
        EventBus::instance().publish<NET_StaConnectedEvent>();
        // Reset connection attempts on success
        // (Handled in run loop by isConnected check)
    }
}

[[noreturn]] void NetworkService::run() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &NetworkService::wifi_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &NetworkService::ip_event_handler, nullptr, nullptr));

    int wifiConnectAttempts = 0;

    for (;;) {
        if (isConnected() && !isInApMode() && !Environment::isTimeSynchronized()) {
            LOG_DEBUG("Restarting SNTP time sync...");
            configTime(0, 0, SNTP_SERVER);
        }

        if (isInApMode() || isConnected()) {
            wifiConnectAttempts = 0; // Reset on AP mode or successful connection
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }

        std::string ssid = Settings::instance().ssid.load().c_str();
        std::string pwd = Settings::instance().pwd.load().c_str();

        if (Environment::isWokwiEmulator()) {
            LOG_DEBUG("Using virtual Wokwi emulator network");
            ssid = "Wokwi-GUEST";
            pwd = "";
        }

        if (ssid.empty()) {
            LOG_DEBUG("No saved SSID, switching to AP mode...");
            enterAPMode();
            wifiConnectAttempts = 0;
        } else {
            EventBus::instance().publish<NET_ConnectingEvent>();
            LOG_DEBUG("Connecting to WiFi... (SSID: %s)", ssid.c_str());

            wifi_config_t wifi_config = {};
            strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), ssid.c_str(), sizeof(wifi_config.sta.ssid));
            strncpy(reinterpret_cast<char *>(wifi_config.sta.password), pwd.c_str(), sizeof(wifi_config.sta.password));
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_start());
            s_isInApMode = false;

            wifiConnectAttempts++;
            if (wifiConnectAttempts >= WIFI_CONNECT_MAX_ATTEMPTS) {
                LOG_ERROR("WiFi connection failed after %d attempts, switching to AP mode...", WIFI_CONNECT_MAX_ATTEMPTS);
                enterAPMode();
                wifiConnectAttempts = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void NetworkService::enterAPMode() {
    wifi_config_t ap_config = {};
    strncpy(reinterpret_cast<char *>(ap_config.ap.ssid), DEFAULT_SSID, sizeof(ap_config.ap.ssid));
    strncpy(reinterpret_cast<char *>(ap_config.ap.password), DEFAULT_PASSWORD, sizeof(ap_config.ap.password));
    ap_config.ap.ssid_len = strlen(DEFAULT_SSID);
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_isInApMode = true;
    EventBus::instance().publish<NET_ApCreatedEvent>();
}

std::string NetworkService::getApIpString() {
    esp_netif_ip_info_t ip_info;
    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif && esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
                 ip4_addr1(&ip_info.ip), ip4_addr2(&ip_info.ip),
                 ip4_addr3(&ip_info.ip), ip4_addr4(&ip_info.ip));
        return std::string(buf);
    }
    return "0.0.0.0";
}
