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
#include "../screens/ApModeScreen.h"
#include "../screens/ClockScreen.h"
#include "../screens/ErrorScreen.h"
#include "../screens/ImageScreen.h"
#include "../screens/Screen.h"
#include "screens/DebugScreen.h"
#include "screens/LoadingScreen.h"
#include "utils/Diagnostics.h"
#include "utils/Environment.h"

DisplayService::DisplayService() : Task("DisplayService", 12288, 2) {}

[[noreturn]] void DisplayService::panic(const char *msg, const char *func, const int line, const char *file) {
    LOG_ERROR("'%s' at %s+%d in %s", msg, func, line, file);

    // If the caller is not the LVGL task, we must kill it to prevent further LVGL operations
    // which could interfere with our panic display.
    if (pcTaskGetName(nullptr) == taskName()) {
        lvglAdapter.panic();
    }

    backlight.wake();
    backlight.setBrightness(1);

    const auto footer = line > 0 ? String(func) + "+" + String(line) : String(func) + "\nin " + String(file);
    tft.panic(msg, footer.c_str());

    Diagnostics::printHeapUsageSafely();
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}


static void anim_x_cb(void * var, int32_t v)
{
    lv_obj_set_x((lv_obj_t *) var, v);
}

static void anim_size_cb(void * var, int32_t v)
{
    lv_obj_set_size((lv_obj_t *) var, v, v);
}

void DisplayService::changeScreen(std::unique_ptr<Screen> newScreen, const unsigned long timeoutSec) {
    // Old Screen object is implicitly deleted here
    currentScreen = std::move(newScreen);
    lv_obj_t* scr = currentScreen->root();
    lv_obj_set_flag(scr, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_max_height(scr, 240, LV_STATE_ANY);
    lv_obj_set_style_max_width(scr, 240, LV_STATE_ANY);
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_NONE, 300, 0, true);

    screenTimeout = timeoutSec == 0 ? 0 : timeoutSec * 1000UL;
    screenSince = millis();
}

void DisplayService::showOverlay(const String& message, const unsigned long duration) {
    lv_obj_t* label = lv_label_create(lv_layer_top());
    lv_obj_set_style_pad_all(label, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_margin_hor(label, 8, LV_STATE_DEFAULT);
    lv_label_set_text(label, message.c_str());
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(label, lv_color_hex(0x444444), LV_STATE_DEFAULT);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_delete_delayed(label, duration);
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
    eventBus.subscribe(EventId::WEB_ShowLocalImage, displayEventQueue);
    eventBus.subscribe(EventId::CFG_Updated, displayEventQueue);
    eventBus.subscribe(EventId::CFG_BrightnessUpdated, displayEventQueue);

    // changeScreen(std::unique_ptr<Screen>(new DebugScreen()), 0);

    if (!NetworkService::isConnected()) {
        changeScreen(std::unique_ptr<Screen>(new LoadingScreen("Connecting...")), 0);
    } else {
        changeScreen(std::unique_ptr<Screen>(new ClockScreen()), 0);
    }

    for (;;) {
        if (EventPtr event = EventBus::tryReceive(displayEventQueue); event != nullptr) {
            switch (event->id()) {
                case EventId::NET_ApCreated:
                    changeScreen(std::unique_ptr<Screen>(new ApModeScreen()), 60);
                    break;
                case EventId::NET_StaConnected:
                    // Wait for SNTP time sync if not done yet
                    if (!Environment::isTimeSynchronized()) {
                        isWaitingForTimeSync = true;
                        changeScreen(std::unique_ptr<Screen>(new LoadingScreen("Syncing time...")), 60);
                    } else {
                        changeScreen(std::unique_ptr<Screen>(new ClockScreen()), 0);
                    }
                    // TODO: show IP
                    showOverlay("WiFi connected", 3000);
                case EventId::API_KeepAlive:
                    if (!backlight.isSleepingByPowerButton()) {
                        backlight.wake();
                    }
                    lastKeepaliveTime = millis();
                    break;
                case EventId::API_ShowImageFromUrl:
                    displayImageFromAPI(event->to<API_ShowImageFromUrlEvent>()->url());
                    break;
                case EventId::WEB_MqttDisconnected: {
                    const auto reason = event->to<WEB_MqttDisconnectedEvent>()->reason();
                    if (reason != AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
                        showOverlay("MQTT disconnected", 3000);
                    }
                    break;
                }
                case EventId::WEB_MqttError:
                    changeScreen(std::unique_ptr<Screen>(new ErrorScreen(event->to<WEB_MqttErrorEvent>()->message())), 30);
                    break;
                case EventId::CFG_Updated:
                    showOverlay("Data updated", 10000);
                    break;
                case EventId::CFG_BrightnessUpdated:
                    backlight.setBrightness(event->to<CFG_BrightnessUpdatedEvent>()->brightness());
                    break;
                default:
                    break;
            }
        }

        if (isWaitingForTimeSync && Environment::isTimeSynchronized()) {
            isWaitingForTimeSync = false;
            changeScreen(std::unique_ptr<Screen>(new ClockScreen()), 0);
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

void DisplayService::displayImageFromAPI(const String &url) {
    constexpr int maxTries = 3;
    int tries = 0;
    bool success = false;
    String lastErrorReason = "";
    constexpr size_t MAX_FILE_SIZE = 70 * 1024;

    const String detectionId = url.substring(url.lastIndexOf("/events/") + 8, url.indexOf("/snapshot.jpg"));
    const int dashIndex = detectionId.indexOf("-");
    const String suffix = (dashIndex > 0) ? detectionId.substring(dashIndex + 1) : detectionId;

    String filename = "/events/" + suffix + "-default.jpg";
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
