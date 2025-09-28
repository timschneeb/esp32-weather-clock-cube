#include "DisplayService.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <memory>

#include "Config.h"
#include "Settings.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "services/NetworkService.h"
#include "services/display/Screen.h"
#include "LvglTask.h"
#include "services/display/ClockScreen.h"
#include "services/display/ErrorScreen.h"
#include "services/display/StatusScreen.h"
#include "services/display/ImageScreen.h"
#include "services/display/ApModeScreen.h"

static auto tft = TFT_eSPI();

static void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    const uint32_t w = (area->x2 - area->x1 + 1);
    const uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(reinterpret_cast<uint16_t *>(color_p), w * h, true);
    tft.endWrite();

    tft.endWrite();
}

DisplayService::DisplayService() : Task("DisplayService", 12288, 2), disp_drv(), draw_buf(),
                                   buf1(nullptr), buf2(nullptr) {}

[[noreturn]] void DisplayService::panic(const char *msg, const char *func, const int line, const char *file) {
    backlight.wake();
    backlight.setBrightness(255);
    
    lv_obj_clean(lv_scr_act());
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xff0000), LV_PART_MAIN);

    lv_obj_t* label = lv_label_create(scr);
    lv_label_set_text(label, "PANIC");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* msg_label = lv_label_create(scr);
    lv_label_set_text_fmt(msg_label, "%s\n\n%s+%d\n%s", msg, func, line, file);
    lv_obj_set_style_text_color(msg_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, 0);

    while (true) {
        lv_timer_handler();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void DisplayService::setScreen(std::unique_ptr<Screen> newScreen, const unsigned long timeoutSec) {
    currentScreen = std::move(newScreen);
    lv_obj_t* scr = lv_obj_create(nullptr);
    currentScreen->draw(scr);
    lv_scr_load(scr);
}

void DisplayService::showOverlay(const String& message, const unsigned long duration) {
    lv_obj_t* label = lv_label_create(lv_layer_top());
    lv_label_set_text(label, message.c_str());
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_timer_t* timer = lv_timer_create([](lv_timer_t* timer) {
        lv_obj_del_async(static_cast<lv_obj_t *>(timer->user_data));
    }, duration, label);
    lv_timer_set_repeat_count(timer, 1);
}

void DisplayService::displayImageFromAPI(const String &url, const String &zone) {
    constexpr int maxTries = 3;
    int tries = 0;
    bool success = false;
    String lastErrorReason = "";
    constexpr size_t MAX_FILE_SIZE = 70 * 1024;

    String detectionId = url.substring(url.lastIndexOf("/events/") + 8, url.indexOf("/snapshot.jpg"));
    int dashIndex = detectionId.indexOf("-");
    String suffix = (dashIndex > 0) ? detectionId.substring(dashIndex + 1) : detectionId;

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
        int httpCode = http.GET();
        if (httpCode == 200) {
            uint32_t len = http.getSize();
            if (len > MAX_FILE_SIZE) {
                setScreen(std::unique_ptr<Screen>(new ErrorScreen("Image too large")), 5);
                http.end();
                return;
            }

            File file = SPIFFS.open(filename, FILE_WRITE);
            if (file) {
                size_t written = http.writeToStream(&file);
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
    tft.begin();
    tft.setRotation(0);
    // Ensure TFT_eSPI expects LVGL's little-endian color order
    tft.setSwapBytes(true);

    button.begin();
    button.attachClick([this] {
        backlight.handlePowerButton();
        lastKeepaliveTime = 0;
    });

    backlight.setBrightness(Settings::instance().brightness);

    lv_init();

    constexpr uint32_t buf_pixels = 240 * 10; // 10 lines buffer
    buf1 = static_cast<lv_color_t *>(heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA));
    buf2 = static_cast<lv_color_t *>(heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA));
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

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

        static unsigned long screenTimeout = 0;
        static unsigned long screenSince = 0;

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

        // Let LVGL process timers and rendering
        lv_timer_handler();

        button.tick();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}
