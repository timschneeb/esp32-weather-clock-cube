#include "DisplayService.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <lvgl.h>
#include <memory>
#include <SPIFFS.h>
#include <TFT_eSPI.h>

#include "Config.h"
#include "Settings.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "misc/lv_timer.h"
#include "lvgl/LvglDisplayAdapter.h"
#include "services/NetworkService.h"
#include "screens/display/ApModeScreen.h"
#include "screens/display/ClockScreen.h"
#include "screens/display/ErrorScreen.h"
#include "screens/display/ImageScreen.h"
#include "screens/display/Screen.h"

DisplayService::DisplayService() : Task("DisplayService", 12288, 2) {}

[[noreturn]] void DisplayService::panic(const char *msg, const char *func, const int line, const char *file) {
    Serial.println(msg);
    Serial.println("at " + String(func) + "+" + String(line) + " in " + String(file));

    backlight.wake();
    backlight.setBrightness(1);

    const auto footer = line > 0 ? String(func) + "+" + String(line) : String(func) + "\nin " + String(file);
    tft.panic(msg, footer.c_str());
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}

void DisplayService::changeScreen(std::unique_ptr<Screen> newScreen, const unsigned long timeoutSec) {
    currentScreen = std::move(newScreen);
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_set_flag(scr, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_max_height(scr, 240, LV_STATE_ANY);
    lv_obj_set_style_max_width(scr, 240, LV_STATE_ANY);
    currentScreen->draw(scr);
    lv_screen_load(scr);

    screenTimeout = timeoutSec == 0 ? 0 : timeoutSec * 1000UL;
    screenSince = millis();
}

void DisplayService::showOverlay(const String& message, const unsigned long duration) {
    return; // TODO deletion causes crash

    lv_obj_t* label = lv_label_create(lv_layer_top());
    lv_label_set_text(label, message.c_str());
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_timer_create(deleteObjectOnTimer, duration, label);
}

[[noreturn]] void DisplayService::run() {
    button.begin();
    button.attachClick([this] {
        backlight.handlePowerButton();
        lastKeepaliveTime = 0;
    });

    lvglAdapter.init(240, 240);
    lvglAdapter.setOnFlushCallback(std::bind(&TFT::push, tft, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    backlight.setBrightness(Settings::instance().brightness);

    const QueueHandle_t displayEventQueue = xQueueCreate(32, sizeof(EventPtr*));
    EventBus &eventBus = EventBus::instance();
    eventBus.subscribe(EventId::NET_StaConnected, displayEventQueue);
    eventBus.subscribe(EventId::NET_ApCreated, displayEventQueue);
    eventBus.subscribe(EventId::API_KeepAlive, displayEventQueue);
    eventBus.subscribe(EventId::API_ShowImageFromUrl, displayEventQueue);
    eventBus.subscribe(EventId::WEB_MqttDisconnected, displayEventQueue);
    eventBus.subscribe(EventId::WEB_MqttError, displayEventQueue);
    eventBus.subscribe(EventId::WEB_ShowImageFromUrlWithZone, displayEventQueue);
    eventBus.subscribe(EventId::WEB_ShowLocalImage, displayEventQueue);
    eventBus.subscribe(EventId::CFG_Updated, displayEventQueue);
    eventBus.subscribe(EventId::CFG_WeatherUpdated, displayEventQueue);

    changeScreen(std::unique_ptr<Screen>(new ClockScreen()), 0);

    for (;;) {
        if (EventPtr event = EventBus::tryReceive(displayEventQueue); event != nullptr) {
            switch (event->id()) {
                case EventId::NET_ApCreated:
                    changeScreen(std::unique_ptr<Screen>(new ApModeScreen()), 86400);
                    break;
                case EventId::NET_StaConnected:
                    showOverlay("WiFi connected", 3000);
                case EventId::API_KeepAlive:
                    if (!backlight.isSleepingByPowerButton()) {
                        backlight.wake();
                    }
                    lastKeepaliveTime = millis();
                    break;
                case EventId::API_ShowImageFromUrl:
                    displayImageFromAPI(event->to<API_ShowImageFromUrlEvent>()->url(), "");
                    break;
                case EventId::WEB_ShowImageFromUrlWithZone:
                    displayImageFromAPI(event->to<WEB_ShowImageFromUrlWithZoneEvent>()->url(), event->to<WEB_ShowImageFromUrlWithZoneEvent>()->zone());
                    break;
                case EventId::WEB_MqttDisconnected: {
                    const auto reason = event->to<WEB_MqttDisconnectedEvent>()->reason();
                    if (reason != AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
                        changeScreen(std::unique_ptr<Screen>(new ErrorScreen("MQTT Disconnected")), 50);
                    }
                    break;
                }
                case EventId::WEB_MqttError:
                    changeScreen(std::unique_ptr<Screen>(new ErrorScreen(event->to<WEB_MqttErrorEvent>()->message())), 30);
                    break;
                case EventId::CFG_Updated:
                case EventId::CFG_WeatherUpdated:
                    showOverlay("Data updated", 10000);
                    break;
                default:
                    break;
            }
        }

        if (currentScreen) {
            if (screenTimeout > 0 && millis() - screenSince > screenTimeout) {
                changeScreen(std::unique_ptr<Screen>(new ClockScreen()), 0);
                screenTimeout = 0;
            }
        }

        if (currentScreen) {
            currentScreen->update();
        }

        if (lastKeepaliveTime > 0 && millis() - lastKeepaliveTime > KEEPALIVE_TIMEOUT) {
            backlight.sleep();
            lastKeepaliveTime = 0;
        }

        lvglAdapter.tick();
        button.tick();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void DisplayService::deleteObjectOnTimer(lv_timer_t* timer) {
    lv_obj_del_async(static_cast<lv_obj_t *>(lv_timer_get_user_data(timer)));
}

void DisplayService::displayImageFromAPI(const String &url, const String &zone) {
    constexpr int maxTries = 3;
    int tries = 0;
    bool success = false;
    String lastErrorReason = "";
    constexpr size_t MAX_FILE_SIZE = 70 * 1024;

    const String detectionId = url.substring(url.lastIndexOf("/events/") + 8, url.indexOf("/snapshot.jpg"));
    const int dashIndex = detectionId.indexOf("-");
    const String suffix = (dashIndex > 0) ? detectionId.substring(dashIndex + 1) : detectionId;

    String filename = "/events/" + suffix + "-" + zone + ".jpg";
    if (filename.length() >= 32) {
        filename = "/events/default.jpg";
    }

    if (SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
    }

    while (tries < maxTries && !success) {
        HTTPClient http;
        http.begin(url);
        if (const int httpCode = http.GET(); httpCode == 200) {
            const uint32_t len = http.getSize();
            if (len > MAX_FILE_SIZE) {
                changeScreen(std::unique_ptr<Screen>(new ErrorScreen("Image too large")), 5);
                http.end();
                return;
            }

            fs::File file = SPIFFS.open(filename, FILE_WRITE);
            if (file) {
                const size_t written = http.writeToStream(&file);
                file.close();
                if (written == len) {
                    success = true;
                    changeScreen(std::unique_ptr<Screen>(new ImageScreen(filename)), Settings::instance().displayDuration);
                }
            }
            http.end();
        } else {
            lastErrorReason = HTTPClient::errorToString(httpCode) + ": " + http.getString();
            http.end();
            tries++;
            delay(500);
        }
    }

    if (!success) {
        changeScreen(std::unique_ptr<Screen>(new ErrorScreen("Loading failed: " + lastErrorReason)), 5);
    }
}
