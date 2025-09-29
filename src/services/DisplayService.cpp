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
#include "services/NetworkService.h"
#include "services/display/ApModeScreen.h"
#include "services/display/ClockScreen.h"
#include "services/display/ErrorScreen.h"
#include "services/display/ImageScreen.h"
#include "services/display/Screen.h"

static void lvglFlushToDisplay(lv_display_t * disp, const lv_area_t * area, uint8_t * data) {
    const uint32_t w = area->x2 - area->x1 + 1;
    const uint32_t h = area->y2 - area->y1 + 1;
    DisplayService::instance().pushToDisplay(reinterpret_cast<uint16_t *>(data), area->x1, area->y1, w, h);
    lv_display_flush_ready(disp);
}

static uint32_t lvglGetTicks() {
    return xTaskGetTickCount() / portTICK_PERIOD_MS;
}

DisplayService::DisplayService() : Task("DisplayService", 12288, 2),
                                   disp(nullptr), drawBuffer(), drawBuf1(nullptr), drawBuf2(nullptr) {}

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

void DisplayService::setScreen(std::unique_ptr<Screen> newScreen) {
    currentScreen = std::move(newScreen);
    lv_obj_t* scr = lv_obj_create(nullptr);
    currentScreen->draw(scr);
    lv_scr_load(scr);
}

void DisplayService::showOverlay(const String& message, const unsigned long duration) {
    lv_obj_t* label = lv_label_create(lv_layer_top());
    lv_label_set_text(label, message.c_str());
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_timer_create(deleteObjectOnTimer, duration, label);
}

void DisplayService::deleteObjectOnTimer(lv_timer_t* timer) {
    lv_obj_del_async(static_cast<lv_obj_t *>(lv_timer_get_user_data(timer)));
}

void DisplayService::pushToDisplay(uint16_t *buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    tft.push(buffer, x, y, w, h);
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
                setScreen(std::unique_ptr<Screen>(new ErrorScreen("Image too large")), 5);
                http.end();
                return;
            }

            fs::File file = SPIFFS.open(filename, FILE_WRITE);
            if (file) {
                const size_t written = http.writeToStream(&file);
                file.close();
                if (written == len) {
                    success = true;
                    setScreen(std::unique_ptr<Screen>(new ImageScreen(filename)), Settings::instance().displayDuration);
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
        setScreen(std::unique_ptr<Screen>(new ErrorScreen("Loading failed: " + lastErrorReason)), 5);
    }
}

[[noreturn]] void DisplayService::run() {
    button.begin();
    button.attachClick([this] {
        backlight.handlePowerButton();
        lastKeepaliveTime = 0;
    });

    backlight.setBrightness(Settings::instance().brightness);

    lv_init();
    lv_tick_set_cb(lvglGetTicks);

    disp = lv_display_create(240, 240);
    lv_display_set_flush_cb(disp, lvglFlushToDisplay);

    constexpr uint32_t bufferSize = 240 * 10 * sizeof(uint16_t); // 10 lines buffer
    drawBuf1 = static_cast<uint16_t *>(heap_caps_malloc(bufferSize, MALLOC_CAP_DMA));
    drawBuf2 = static_cast<uint16_t *>(heap_caps_malloc(bufferSize, MALLOC_CAP_DMA));
    lv_display_set_buffers(disp, drawBuf1, drawBuf2, bufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Process LVGL in this task to keep LVGL single-threaded
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

    setScreen(std::unique_ptr<Screen>(new ClockScreen()), 0);

    static unsigned long screenTimeout = 0;
    static unsigned long screenSince = 0;

    for (;;) {
        if (EventPtr event = EventBus::tryReceive(displayEventQueue); event != nullptr) {
            switch (event->id()) {
                case EventId::NET_ApCreated:
                    setScreen(std::unique_ptr<Screen>(new ApModeScreen()), 86400);
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
                        setScreen(std::unique_ptr<Screen>(new ErrorScreen("MQTT Disconnected")), 50);
                    }
                    break;
                }
                case EventId::WEB_MqttError:
                    setScreen(std::unique_ptr<Screen>(new ErrorScreen(event->to<WEB_MqttErrorEvent>()->message())), 30);
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
                setScreen(std::unique_ptr<Screen>(new ClockScreen()), 0);
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

        lv_timer_handler();
        button.tick();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}
