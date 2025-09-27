#include "DisplayService.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <TJpg_Decoder.h>

#include "Config.h"
#include "Settings.h"
#include "display/ApModeScreen.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "services/NetworkService.h"
#include "display/ImageScreen.h"

DisplayService::DisplayService() : Task("DisplayService", 8192, 2) {}

void DisplayService::panic(const char *msg, const char *func, const int line, const char *file) {
    backlight.wake();
    backlight.setBrightness(255);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(3);
    tft.println("=== PANIC ===");
    tft.setTextColor(TFT_ORANGE);
    tft.setTextSize(2);
    tft.println();
    tft.println(msg);
    tft.println();
    tft.println(line > 0 ? String(func) + "+" + String(line) : String(func));
    tft.println("in " + String(file));
    tft.flush();
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}

bool DisplayService::jpgRenderCallback(const int16_t x, const int16_t y, const uint16_t w, const uint16_t h, uint16_t *bitmap) {
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void DisplayService::handleSlideshow() {
    const unsigned long now = millis();

    if (!slideshowActive || jpgQueue.empty()) {
        slideshowActive = false;
        return;
    }

    if (now - slideshowStart >= Settings::instance().displayDuration * 1000UL) {
        slideshowActive = false;
        jpgQueue.clear();
        eventCallTimes.clear();
        setScreen(std::unique_ptr<ClockScreen>(new ClockScreen()), 0);
        return;
    }

    if (now - slideshowStart >= currentSlideshowIdx * Settings::instance().slideshowInterval) {
        const String& filename = jpgQueue[currentSlideshowIdx % jpgQueue.size()];
        setScreen(std::unique_ptr<ImageScreen>(new ImageScreen(filename)), 0);
        currentSlideshowIdx++;
    }
}

void DisplayService::setScreen(std::unique_ptr<Screen> newScreen, const unsigned long timeoutSec) {
    currentScreen = std::move(newScreen);
    currentScreen->draw(tft);
    screenTimeout = timeoutSec == 0 ? 0 : timeoutSec * 1000UL;
    screenSince = millis();
}

void DisplayService::showOverlay(const String& message, const unsigned long duration) {
    overlayScreen.reset(new StatusScreen(message, duration));
    overlayScreen->draw(tft);
}

void DisplayService::displayImageFromAPI(const String &url, const String &zone) {
    const auto displayDuration = Settings::instance().displayDuration.load();
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
                setScreen(std::unique_ptr<ErrorScreen>(new ErrorScreen("Image too large")), 5);
                http.end();
                return;
            }

            String oldestFile = "";
            unsigned long oldestTime = ULONG_MAX;
            File root = SPIFFS.open("/events");
            if (root && root.isDirectory()) {
                int jpgCount = 0;
                File file = root.openNextFile();
                while (file) {
                    String fname = file.name();
                    if (fname.endsWith(".jpg")) {
                        jpgCount++;
                        if (!fname.startsWith("/")) {
                            fname = "/events/" + fname;
                        }
                        unsigned long mtime = file.getLastWrite();
                        if (mtime < oldestTime) {
                            oldestTime = mtime;
                            oldestFile = fname;
                        }
                    }
                    file = root.openNextFile();
                }
                root.close();
                if (jpgCount >= Settings::instance().maxImages && !oldestFile.isEmpty()) {
                    SPIFFS.remove(oldestFile);
                }
            }

            WiFiClient *stream = http.getStreamPtr();
            auto *jpgData = static_cast<uint8_t *>(malloc(len));
            if (jpgData == nullptr) {
                http.end();
                return;
            }

            if (stream->available() != 0) {
                size_t bytesRead = stream->readBytes(reinterpret_cast<char *>(jpgData), len);
                File file = SPIFFS.open(filename, FILE_WRITE);
                if (file) {
                    size_t written = file.write(jpgData, len);
                    file.close();
                    if (written == len) {
                        success = true;
                        if (std::find(jpgQueue.begin(), jpgQueue.end(), filename) == jpgQueue.end()) {
                            jpgQueue.push_back(filename);
                        }
                        setScreen(std::unique_ptr<ImageScreen>(new ImageScreen(filename)), displayDuration);
                    }
                }
                free(jpgData);
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
        setScreen(std::unique_ptr<ErrorScreen>(new ErrorScreen("Loading failed: " + lastErrorReason)), 5);
    }
}

[[noreturn]] void DisplayService::run() {
    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    button.begin();
    button.attachClick([this] {
        backlight.handlePowerButton();
        lastKeepaliveTime = 0;
    });

    backlight.setBrightness(Settings::instance().brightness);

    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback([](const int16_t x, const int16_t y, const uint16_t w, const uint16_t h, uint16_t *bitmap) {
        return instance().jpgRenderCallback(x, y, w, h, bitmap);
    });

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

    setScreen(std::unique_ptr<ClockScreen>(new ClockScreen()), 0);

    for (;;) {
        if (imagePending) {
            imagePending = false;
            displayImageFromAPI(pendingImageUrl, pendingZone);
        }

        if (slideshowActive) {
            handleSlideshow();
        }

        EventPtr event = EventBus::tryReceive(displayEventQueue);
        if (event != nullptr) {
            switch (event->id()) {
                case EventId::NET_StaConnected:
                    showOverlay("WiFi Connected\n" + WiFi.localIP().toString(), 5000);
                    break;
                case EventId::NET_ApCreated:
                    setScreen(std::unique_ptr<ApModeScreen>(new ApModeScreen()), 86400);
                    break;
                case EventId::API_KeepAlive:
                    lastKeepaliveTime = event->to<API_KeepAliveEvent>()->now();
                    if (backlight.isSleeping() && !backlight.isSleepingByPowerButton()) {
                        backlight.wake();
                    }
                    break;
                case EventId::API_ShowImageFromUrl:
                    pendingImageUrl = event->to<API_ShowImageFromUrlEvent>()->url();
                    imagePending = true;
                    break;
                case EventId::WEB_MqttDisconnected: {
                    const auto reason = event->to<WEB_MqttDisconnectedEvent>()->reason();
                    if (reason != AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
                        setScreen(std::unique_ptr<ErrorScreen>(new ErrorScreen("MQTT Disconnected")), 50);
                    }
                    break;
                }
                case EventId::WEB_MqttError:
                    setScreen(std::unique_ptr<ErrorScreen>(new ErrorScreen(event->to<WEB_MqttErrorEvent>()->message())), 30);
                    break;
                case EventId::WEB_ShowImageFromUrlWithZone:
                    pendingImageUrl = event->to<WEB_ShowImageFromUrlWithZoneEvent>()->url();
                    pendingZone = event->to<WEB_ShowImageFromUrlWithZoneEvent>()->zone();
                    imagePending = true;
                    break;
                case EventId::WEB_ShowLocalImage: {
                    const auto file = event->to<WEB_ShowLocalImageEvent>()->filename();
                    if (std::find(jpgQueue.begin(), jpgQueue.end(), file) == jpgQueue.end()) {
                        jpgQueue.push_back(file);
                    }
                    break;
                }
                case EventId::CFG_Updated:
                case EventId::CFG_WeatherUpdated:
                    showOverlay("Data updated", 10000);
                    // The clock screen will pick up the changes automatically
                    break;
                default: ;
            }
        }

        if (screenTimeout > 0 && millis() - screenSince > screenTimeout) {
            setScreen(std::unique_ptr<ClockScreen>(new ClockScreen()), 0);
        }

        if (currentScreen) {
            currentScreen->update(tft);
        }

        if (overlayScreen) {
            if (overlayScreen->isExpired()) {
                overlayScreen.reset();
                // Redraw current screen to remove overlay
                if(currentScreen) currentScreen->draw(tft);
            } else {
                overlayScreen->draw(tft);
                overlayScreen->update(tft);
            }
        }

        if (lastKeepaliveTime > 0 && millis() - lastKeepaliveTime > KEEPALIVE_TIMEOUT) {
            backlight.sleep();
            lastKeepaliveTime = 0;
        }

        button.tick();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}