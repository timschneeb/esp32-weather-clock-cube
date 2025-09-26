//
// Created by tim on 24.09.25.
//

#include "NetworkService.h"

#include <QuarkTS.h>
#include <WiFi.h>

#include "Config.h"
#include "Events.h"
#include "Settings.h"
#include "TFT_eSPI.h"

constexpr auto WIFI_TIMEOUT = 25000;

NetworkService::NetworkService() = default;

void NetworkService::registerTask() {
    qOS::os.add(instance(), nullptr, qOS::core::MEDIUM_PRIORITY, 10000, PERIODIC);
    qOS::os.notify(qOS::notifyMode::SIMPLE, instance(), nullptr);
    instance().setName("Network");
}

String NetworkService::getSavedSSID() {
    return Settings::instance().ssid;
}

void NetworkService::connectToSavedWiFi() {
    qOS::os.notify(qOS::notifyMode::QUEUED, *this, new NET_ConnectStaNowEvent());
}

bool NetworkService::isConnected() {
    return WiFiClass::status() == WL_CONNECTED;
}

void NetworkService::activities(qOS::event_t e) {
    Serial.println(">>>>> [NETWORK] Service running: " + String(static_cast<int>(e.getTrigger())));
    if (WiFiClass::status() == WL_CONNECTED) {
        return;
    }

    auto forceConnectSta = false;
    if (e.getTrigger() == qOS::trigger::byNotificationQueued) {
        const auto event = static_cast<IEvent *>(e.EventData);
        if (event != nullptr) {
            if (event->id() == EventId::NET_ConnectStaNow) {
                forceConnectSta = true;
            }
            delete event;
        }
    }

    String ssid = Settings::instance().ssid;
    String pwd = Settings::instance().pwd;

    if (ssid == "") {
        qOS::logger::out() << "No saved SSID, switching to AP mode..." << qOS::logger::endl;
        enterAPMode();
    } else if (!isInApMode() || forceConnectSta) {
        WiFiClass::mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pwd.c_str());
        WiFi.setAutoReconnect(true);
        //qOS::os.notify(qOS::notifyMode::QUEUED, new NET_ConnectingEvent());

        qOS::logger::out() << "Connecting to WiFi..." << qOS::logger::endl;

        while (WiFiClass::status() != WL_CONNECTED) {
            qOS::logger::out() << "Connecting to WiFi..." << qOS::logger::endl;
#undef delay
            delay(100);
        }

        Serial.println("Wifi loop done");

        if (WiFiClass::status() != WL_CONNECTED) {
            qOS::logger::out() << "WiFi failed, fallback to AP mode..." << qOS::logger::endl;
            enterAPMode();
            qOS::os.notify(qOS::notifyMode::QUEUED, new NET_ApCreatedEvent());
        } else {
            qOS::logger::out() << "WiFi connected to: " << Settings::instance().ssid << qOS::logger::endl;
            qOS::logger::out() << "IP address: " << WiFi.localIP().toString() << qOS::logger::endl;
            qOS::os.notify(qOS::notifyMode::QUEUED, new NET_StaConnectedEvent());
        }
    }
    task::activities(e);
}

bool NetworkService::isInApMode() {
    return WiFiClass::getMode() == WIFI_MODE_AP;
}

void NetworkService::enterAPMode() {
    WiFiClass::mode(WIFI_AP);
    WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
}
