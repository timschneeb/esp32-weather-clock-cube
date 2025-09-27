#include "DisplayService.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <TJpg_Decoder.h>

#include "Config.h"
#include "Settings.h"
#include "event/EventBus.h"
#include "event/Events.h"
#include "services/NetworkService.h"

// Constants
constexpr unsigned long CLOCK_REFRESH_INTERVAL = 500UL;
const char *daysShort[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {
    "Jan.", "Feb.", "March", "April", "May", "June", "July", "August", "Sept.", "Oct.", "Nov.", "Dec."
};

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

    // Check if slideshow is active and jpgQueue is not empty
    if (!slideshowActive || jpgQueue.empty()) {
        slideshowActive = false;
        return;
    }

    // Check if displayDuration has elapsed
    if (now - slideshowStart >= Settings::instance().displayDuration * 1000UL) {
        Serial.println("[SLIDESHOW] Display duration expired, stopping slideshow");
        slideshowActive = false;
        jpgQueue.clear();
        eventCallTimes.clear();
        setScreen("clock", 0, "slideshow timeout");
        return;
    }

    // Check if it's time for the next image
    if (now - slideshowStart >= currentSlideshowIdx * Settings::instance().slideshowInterval) {
        const String& filename = jpgQueue[currentSlideshowIdx % jpgQueue.size()];
        if (SPIFFS.exists(filename)) {
            File file = SPIFFS.open(filename, FILE_READ);
            if (file) {
                const auto fileSize = file.size();
                auto *jpgData = static_cast<uint8_t *>(malloc(fileSize));
                if (jpgData != nullptr) {
                    const size_t bytesRead = file.readBytes(reinterpret_cast<char *>(jpgData), fileSize);
                    file.close();
                    if (bytesRead == fileSize) {
                        tft.fillScreen(TFT_BLACK);
                        TJpgDec.drawJpg(0, 0, jpgData, fileSize);
                        Serial.println("[SLIDESHOW] Displayed: " + filename);
                    }
                    free(jpgData);
                } else {
                    Serial.println("[SLIDESHOW] Memory allocation failed for: " + filename);
                }
            } else {
                Serial.println("[SLIDESHOW] Cannot open file: " + filename);
            }
        } else {
            Serial.println("[SLIDESHOW] Image not found: " + filename);
        }

        currentSlideshowIdx++;
    }
}

void DisplayService::setScreen(const String &newScreen, const unsigned long timeoutSec, const char *by) {
    Serial.printf("setScreen: from %s to %s (timeout: %lu sec) by: %s\n", currentScreen.c_str(), newScreen.c_str(),
                  timeoutSec, by);

    auto displayDuration = Settings::instance().displayDuration.load();
    // Remove old event calls outside displayDuration
    unsigned long now = millis();
    eventCallTimes.erase(
        std::remove_if(eventCallTimes.begin(), eventCallTimes.end(),
                       [now, displayDuration](const unsigned long t) { return now - t > displayDuration * 1000UL; }),
        eventCallTimes.end()
    );

    // Handle "event" screen
    if (newScreen == "event") {
        // Add current time to eventCallTimes
        eventCallTimes.push_back(now);
        lastEventCall = now;

        // Check if slideshow should be started (more than 1 call within displayDuration)
        if (eventCallTimes.size() > 1 && !slideshowActive) {
            Serial.println("[SLIDESHOW] Starting slideshow due to multiple event calls");
            slideshowActive = true;
            slideshowStart = now;
            currentSlideshowIdx = 0;
        }

        // If slideshow is active, delegate to handleSlideshow
        if (slideshowActive && !jpgQueue.empty()) {
            handleSlideshow();
        } else if (!slideshowActive && !jpgQueue.empty()) {
            // Display single image
            const String& filename = jpgQueue[0];
            if (SPIFFS.exists(filename)) {
                File file = SPIFFS.open(filename, FILE_READ);
                if (file) {
                    const uint32_t fileSize = file.size();
                    auto *jpgData = static_cast<uint8_t *>(malloc(fileSize));
                    if (jpgData != nullptr) {
                        const size_t bytesRead = file.readBytes(reinterpret_cast<char *>(jpgData), fileSize);
                        file.close();
                        if (bytesRead == fileSize) {
                            //tft.fillScreen(TFT_BLACK);
                            TJpgDec.drawJpg(0, 0, jpgData, fileSize);
                            Serial.println("[DEBUG] Displayed single image: " + filename);
                        }
                        free(jpgData);
                    } else {
                        Serial.println("[DEBUG] Cannot open file: " + filename);
                    }
                } else {
                    Serial.println("[DEBUG] Image not found: " + filename);
                }
            }
        }

        currentScreen = "event";
        screenTimeout = timeoutSec == 0 ? 0 : timeoutSec * 1000UL;
        screenSince = now;
        return;
    }

    // Handle other screens (clock, error, etc.)
    if (currentScreen != newScreen) {
        currentScreen = newScreen;
        tft.fillScreen(TFT_BLACK);

        if (newScreen == "clock") {
            lastDrawnWeatherIcon = "";
            lastDate = "";
            slideshowActive = false;
            jpgQueue.clear();
            eventCallTimes.clear();
        }
    }
    screenTimeout = timeoutSec == 0 ? 0 : timeoutSec * 1000UL;
    screenSince = now;
}

void DisplayService::showWeatherIconJPG(const String& iconCode) {
    const String path = "/icons/" + iconCode + ".jpg";
    Serial.print("Search icon: ");
    Serial.println(path);
    constexpr int iconWidth = 90;
    constexpr int iconHeight = 90;
    constexpr int x = 240 - iconWidth - 8;
    constexpr int y = 240 - iconHeight - 8;
    if (SPIFFS.exists(path)) {
        TJpgDec.drawJpg(x, y, path.c_str());
        Serial.print("[WEATHER] Icon drawn: ");
        Serial.println(path);
    } else {
        Serial.print("[WEATHER] Icon NOT found: ");
        Serial.println(path);
        constexpr int pad = 10;
        tft.drawLine(x + pad, y + pad, x + iconWidth - pad, y + iconHeight - pad, TFT_RED);
        tft.drawLine(x + iconWidth - pad, y + pad, x + pad, y + iconHeight - pad, TFT_RED);
        tft.drawRect(x, y, iconWidth, iconHeight, TFT_RED);
    }
}

void DisplayService::showClock() {
    bool isScreenTransition = (currentScreen != "clock");
    if (isScreenTransition) {
        tft.fillScreen(TFT_BLACK);
    }

    time_t now = time(nullptr);
    if (now < 100000) {
        Serial.println("[showClock] Time not yet synchronized.");
    }
    tm *tm_info = localtime(&now);
    if (!tm_info) return;

    // Date
    String enDate = String(daysShort[tm_info->tm_wday]) + " " +
                    String(tm_info->tm_mday) + " " +
                    String(months[tm_info->tm_mon]);
    if (isScreenTransition || enDate != lastDate) {
        tft.fillRect(0, 0, 240, 25, TFT_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int dateWidth = tft.textWidth(enDate);
        int dateX = (240 - dateWidth) / 2;
        tft.setCursor(dateX, 0);
        tft.println(enDate);
        lastDate = enDate;
    }

    // Time
    if (millis() - lastClockUpdate > 1000 || isScreenTransition) {
        lastClockUpdate = millis();
        char timeBuffer[20];
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", tm_info);
        tft.setTextSize(5);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int timeWidth = tft.textWidth(timeBuffer);
        int timeX = (240 - timeWidth) / 2;
        tft.setCursor(timeX, 45);
        tft.println(timeBuffer);
    }

    // Temperature
    auto tempValue = String(Settings::instance().weatherTempDay.load(), 1);
    String tempUnit = "c";
    auto humidityValue = String(static_cast<int>(Settings::instance().weatherHumidity));
    // Geheel getal voor luchtvochtigheid
    String humidityUnit = "% பூ";
    auto tempMinValue = String(Settings::instance().weatherTempMin, 1);
    String tempMinUnit = "c";
    auto tempMaxValue = String(Settings::instance().weatherTempMax, 1);
    String tempMaxUnit = "c";


    tft.drawRect(10, 105, 255, tft.fontHeight(), TFT_BLACK);

    tft.setTextSize(4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 105);
    tft.print(tempValue);
    int tempValueWidth = tft.textWidth(tempValue);
    tft.setTextSize(2);
    tft.setCursor(10 + tempValueWidth, 105);
    tft.print(tempUnit);
    int tempUnitWidth = tft.textWidth(tempUnit);
    tft.setTextSize(4);
    tft.setCursor(10 + tempValueWidth + tempUnitWidth + 5, 105);
    tft.print(humidityValue);
    int humidityValueWidth = tft.textWidth(humidityValue);
    tft.setTextSize(3);
    tft.setCursor(10 + tempValueWidth + tempUnitWidth + 5 + humidityValueWidth, 105);
    tft.print(humidityUnit);

    // Min label
    tft.setTextSize(2);
    tft.setCursor(2, 170);
    tft.print("Min ");
    int minLabelWidth = tft.textWidth("Min ");
    tft.setTextSize(3);
    tft.setCursor(2 + minLabelWidth, 165);
    tft.print(tempMinValue);
    int tempMinValueWidth = tft.textWidth(tempMinValue);
    tft.setTextSize(2);
    tft.setCursor(2 + minLabelWidth + tempMinValueWidth, 170);
    tft.print(tempMinUnit);

    // Max label
    tft.setTextSize(2);
    tft.setCursor(2, 200);
    tft.print("Max ");
    int maxLabelWidth = tft.textWidth("Max ");
    tft.setTextSize(3);
    tft.setCursor(2 + maxLabelWidth, 195);
    tft.print(tempMaxValue);
    int tempMaxValueWidth = tft.textWidth(tempMaxValue);
    tft.setTextSize(2);
    tft.setCursor(2 + maxLabelWidth + tempMaxValueWidth, 200);
    tft.print(tempMaxUnit);


    String weatherIcon = Settings::instance().weatherIcon.load();

    // Weather icon
    if (isScreenTransition || weatherIcon != lastDrawnWeatherIcon) {
        showWeatherIconJPG(weatherIcon);
        lastDrawnWeatherIcon = weatherIcon;
    }
    currentScreen = "clock";
}

void DisplayService::displayImageFromAPI(const String &url, const String &zone) {
    const auto displayDuration = Settings::instance().displayDuration.load();
    constexpr int maxTries = 3;
    int tries = 0;
    bool success = false;
    String lastErrorReason = "";
    constexpr size_t MAX_FILE_SIZE = 70 * 1024;

    // Construct detectionId from URL
    String detectionId = url.substring(url.lastIndexOf("/events/") + 8, url.indexOf("/snapshot.jpg"));
    int dashIndex = detectionId.indexOf("-");
    String suffix = (dashIndex > 0) ? detectionId.substring(dashIndex + 1) : detectionId;

    // New filename: <suffix>-<zone>.jpg
    String filename = "/events/" + suffix + "-" + zone + ".jpg";
    if (filename.length() >= 32) {
        filename = "/events/default.jpg";
    }

    if (SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
    }

    while (tries < maxTries && !success) {
        Serial.print("[DEBUG] Attempt ");
        Serial.print(tries + 1);
        Serial.print("/");
        Serial.println(url);
        HTTPClient http;
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode == 200) {
            uint32_t len = http.getSize();
            if (len > MAX_FILE_SIZE) {
                Serial.println("[ERROR] Image too large: " + String(len) + " bytes");
                setScreen("error", 5, "displayImageFromAPI");
                tft.setTextColor(TFT_RED);
                tft.setTextSize(2);
                tft.setCursor(0, 0);
                tft.println("Image too large");
                http.end();
                return;
            }

            // Remove the oldest image if maxImages is reached
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
                    Serial.println("[DEBUG] Removed: " + oldestFile);
                }
            }

            WiFiClient *stream = http.getStreamPtr();
            auto *jpgData = static_cast<uint8_t *>(malloc(len));
            if (jpgData == nullptr) {
                Serial.println("[ERROR] Memory allocation failed!");
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
                        Serial.println("[DEBUG] Image saved: " + filename);
                        success = true;
                        if (std::find(jpgQueue.begin(), jpgQueue.end(), filename) == jpgQueue.end()) {
                            jpgQueue.push_back(filename);
                        }
                        setScreen("event", displayDuration, "displayImageFromAPI");
                    }
                }
                free(jpgData);
            }
            http.end();
        } else {
            Serial.println("[WARNING] HTTP GET failed: " + String(httpCode));
            lastErrorReason = HTTPClient::errorToString(httpCode) + ": " + http.getString();
            http.end();
            tries++;
            delay(500);
        }
    }

    if (!success) {
        Serial.println("[ERROR] Failed to load image after " + String(maxTries) + " attempts");
        setScreen("error", 5, "displayImageFromAPI");
        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
        tft.setCursor(0, 0);
        tft.println("Loading failed");
        tft.println(lastErrorReason);
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

    TJpgDec.setCallback([](const int16_t x, const int16_t y, const uint16_t w, const uint16_t h, uint16_t *bitmap) {
        return instance().jpgRenderCallback(x, y, w, h, bitmap);
    });
    TJpgDec.setSwapBytes(true);

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
                    setScreen("statusWiFi", 5, "show_wifi_status");
                    tft.setTextColor(TFT_GREEN, TFT_BLACK);
                    tft.setTextSize(2);
                    tft.setCursor(10, 40);
                    tft.println("Connected to:");
                    tft.setCursor(10, 70);
                    tft.println(NetworkService::getSavedSSID());
                    tft.setCursor(10, 110);
                    tft.println("IP:");
                    tft.setCursor(10, 140);
                    tft.println(WiFi.localIP());
                    break;
                case EventId::NET_ApCreated:
                    setScreen("apmode", 86400, "fallbackAP");
                    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
                    tft.setTextSize(2);
                    tft.setCursor(10, 30);
                    tft.println("WiFi not connected");
                    tft.setTextSize(3);
                    tft.setCursor(20, 60);
                    tft.println("**AP MODE**");
                    tft.setTextSize(2);
                    tft.setCursor(10, 110);
                    tft.println("SSID: " + String(DEFAULT_SSID));
                    tft.setCursor(10, 140);
                    tft.println("PWD: " + String(DEFAULT_PASSWORD));
                    tft.setCursor(10, 170);
                    tft.println("IP: " + WiFi.softAPIP().toString());
                    break;
                case EventId::API_KeepAlive:
                    lastKeepaliveTime = event->to<API_KeepAliveEvent>()->now();
                    // Wake up if sleeping (however, the power button takes precedence)
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
                    if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
                        // Disconnected from broker, will auto-retry
                        break;
                    }
                    setScreen("error", 50, "onMqttDisconnect");
                    tft.setTextColor(TFT_RED, TFT_BLACK);
                    tft.setTextSize(2);
                    tft.setCursor(0, 0);
                    tft.println("MQTT lost!\nCode: " + String(static_cast<int>(reason)));
                    break;
                }
                case EventId::WEB_MqttError:
                    setScreen("error", 30, "onMqttMessage");
                    tft.setTextColor(TFT_RED);
                    tft.setTextSize(2);
                    tft.setCursor(0, 0);
                    tft.println(event->to<WEB_MqttErrorEvent>()->message());
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
                    break;
                case EventId::CFG_WeatherUpdated:
                    break;
                case EventId::COR_Panic:
                    panic(event->to<COR_PanicEvent>()->details().c_str(), "COR_Panic event", 0, "EventBus");
                    break;
                default: ;
            }
        }

        if (currentScreen != "clock" && screenTimeout > 0 && millis() - screenSince > screenTimeout) {
            setScreen("clock", 0, "timeout");
        }

        if (currentScreen == "clock" && millis() - lastClockUpdate > CLOCK_REFRESH_INTERVAL) {
            showClock();
        }

        if (lastKeepaliveTime > 0 && millis() - lastKeepaliveTime > KEEPALIVE_TIMEOUT) {
            Serial.println(
                "[AutoSleep] Entering automatic sleep mode due to inactivity. Now: " + String(millis()) +
                ", last keepalive: " + String(lastKeepaliveTime));
            backlight.sleep();
            lastKeepaliveTime = 0;
        }

        button.tick();
    }
}
