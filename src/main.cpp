#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <TJpg_Decoder.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ctime>
#include <esp_sntp.h>
#include <ArduinoJson.h>
#include <vector>
#include <algorithm>
// ReSharper disable once CppUnusedIncludeDirective
#include <AsyncHTTPRequest_Generic.h> // Library doesn't handle multiple includes well, needs to be included here

#include "Button.h"
#include "Config.h"
#include "Events.h"
#include "NetworkService.h"
#include "Settings.h"
#include "WeatherService.h"
#include "WebServer.h"
#include "EventBus.h"

// Constants
constexpr int DEBOUNCE_MS = 250;
constexpr unsigned long CLOCK_REFRESH_INTERVAL = 1000UL; // 1 second
const char *daysShort[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {
    "Jan.", "Feb.", "March", "April", "May", "June", "July", "August", "Sept.", "Oct.", "Nov.", "Dec."
};

// Variables
// QueueHandle_t eventQueue; // Replaced by EventBus

String lastDrawnWeatherIcon = "";
String lastDate = "";
String currentScreen = "clock"; // ["clock", "event", "status", "error"]
TFT_eSPI tft = TFT_eSPI();
Button button = Button();

unsigned long lastClockUpdate = 0;
unsigned long lastKeyTime = 0;
unsigned long screenTimeout = 0;
unsigned long screenSince = 0;
boolean sntp_time_was_setup = false;
unsigned long last_keepalive_time = 0;
boolean is_manually_sleeping = false;
boolean is_automatically_sleeping = false;
// --- Slideshow ---
String pendingImageUrl = "";
bool imagePending = false;
String pendingZone = "";
bool slideshowActive = false;
unsigned long slideshowStart = 0;
int currentSlideshowIdx = 0;
std::vector<String> jpgQueue; // Array of full paths to JPG files
std::vector<unsigned long> eventCallTimes;
unsigned long lastEventCall = 0;

// ------------------------
//  Forward Declarations
// ------------------------
void setScreen(const String &newScreen, unsigned long timeoutSec = 0, const char *by = "");

// ------------------------
//  Backlight
// ------------------------
void set_tft_brt(int brt) {
    // prevent setting brightness when sleeping
    if (is_manually_sleeping || is_automatically_sleeping)
        brt = 0;

    brt = constrain(brt, 0, 255);

    ledcWrite(TFT_BL_PWM_CHANNEL, 255 - brt);
}

// ------------------------
//  JPEG render callback
// ------------------------
bool jpgRenderCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

// ------------------------
//  Slideshow handler
// ------------------------
void handleSlideshow() {
    unsigned long now = millis();

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
        String filename = jpgQueue[currentSlideshowIdx % jpgQueue.size()];
        if (SPIFFS.exists(filename)) {
            File file = SPIFFS.open(filename, FILE_READ);
            if (file) {
                uint32_t fileSize = file.size();
                uint8_t *jpgData = (uint8_t *) malloc(fileSize);
                if (jpgData) {
                    size_t bytesRead = file.readBytes((char *) jpgData, fileSize);
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

// ------------------------
//  STATE-based screen management
// ------------------------
void setScreen(const String &newScreen, unsigned long timeoutSec, const char *by) {
    Serial.printf("setScreen: from %s to %s (timeout: %lu sec) by: %s\n", currentScreen.c_str(), newScreen.c_str(),
                  timeoutSec, by);

    auto displayDuration = Settings::instance().displayDuration.load();
    // Remove old event calls outside displayDuration
    unsigned long now = millis();
    eventCallTimes.erase(
        std::remove_if(eventCallTimes.begin(), eventCallTimes.end(),
                       [now, displayDuration](unsigned long t) { return now - t > displayDuration * 1000UL; }),
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
            String filename = jpgQueue[0];
            if (SPIFFS.exists(filename)) {
                File file = SPIFFS.open(filename, FILE_READ);
                if (file) {
                    uint32_t fileSize = file.size();
                    uint8_t *jpgData = (uint8_t *) malloc(fileSize);
                    if (jpgData) {
                        size_t bytesRead = file.readBytes((char *) jpgData, fileSize);
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
        screenTimeout = (timeoutSec == 0) ? 0 : timeoutSec * 1000UL;
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
    screenTimeout = (timeoutSec == 0) ? 0 : timeoutSec * 1000UL;
    screenSince = now;
}

// ------------------------
//  Weather icon
// ------------------------
void showWeatherIconJPG(String iconCode) {
    String path = "/icons/" + iconCode + ".jpg";
    Serial.print("Search icon: ");
    Serial.println(path);
    int iconWidth = 90;
    int iconHeight = 90;
    int x = 240 - iconWidth - 8;
    int y = 240 - iconHeight - 8;
    if (SPIFFS.exists(path)) {
        TJpgDec.drawJpg(x, y, path.c_str());
        Serial.print("[WEATHER] Icon drawn: ");
        Serial.println(path);
    } else {
        Serial.print("[WEATHER] Icon NOT found: ");
        Serial.println(path);
        int pad = 10;
        tft.drawLine(x + pad, y + pad, x + iconWidth - pad, y + iconHeight - pad, TFT_RED);
        tft.drawLine(x + iconWidth - pad, y + pad, x + pad, y + iconHeight - pad, TFT_RED);
        tft.drawRect(x, y, iconWidth, iconHeight, TFT_RED);
    }
}

// ------------------------
//  Clock display
// ------------------------
void showClock() {
    bool isScreenTransition = (currentScreen != "clock");
    if (isScreenTransition) {
        tft.fillScreen(TFT_BLACK);
    }

    time_t now = time(nullptr);
    if (now < 100000) {
        Serial.println("[showClock] Time not yet synchronized.");
    }
    struct tm *tm_info = localtime(&now);
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
    String tempValue = String(Settings::instance().weatherTempDay.load(), 1);
    String tempUnit = "÷c";
    String humidityValue = String(static_cast<int>(Settings::instance().weatherHumidity));
    // Geheel getal voor luchtvochtigheid
    String humidityUnit = "%";
    String tempMinValue = String(Settings::instance().weatherTempMin, 1);
    String tempMinUnit = "÷c";
    String tempMaxValue = String(Settings::instance().weatherTempMax, 1);
    String tempMaxUnit = "÷c";


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

// ------------------------
//  Display image from API
// ------------------------
void displayImageFromAPI(const String &url, const String &zone) {
    const auto displayDuration = Settings::instance().displayDuration.load();
    const int maxTries = 3;
    int tries = 0;
    bool success = false;
    String lastErrorReason = "";
    const size_t MAX_FILE_SIZE = 70 * 1024;

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

            // Remove oldest image if maxImages is reached
            int jpgCount = 0;
            String oldestFile = "";
            unsigned long oldestTime = ULONG_MAX;
            File root = SPIFFS.open("/events");
            if (root && root.isDirectory()) {
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
            if (!jpgData) {
                Serial.println("[ERROR] Memory allocation failed!");
                http.end();
                return;
            }

            if (stream->available()) {
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

void eventHandlerTask(void *pvParameters) {
    QueueHandle_t displayEventQueue = xQueueCreate(20, sizeof(EventPtr*));
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
            Serial.println(
                ">>>> EVENT RECEIVED. ptr: " + String(reinterpret_cast<int>(event.get())) + "; ptr count: " +
                String(event.use_count()) + "; ID: " + String(static_cast<int>(event.get()->id())));

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

                        Serial.println("[TEST] on core " + String(xPortGetCoreID()));
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
                        last_keepalive_time = event->to<API_KeepAliveEvent>()->now();
                        Serial.println("[EHT] Received keepalive at " + String(last_keepalive_time));
                        if (is_manually_sleeping || is_automatically_sleeping) {
                            is_automatically_sleeping = false;
                            is_manually_sleeping = false;
                            set_tft_brt(Settings::instance().brightness);
                        }
                        break;
                    case EventId::API_ShowImageFromUrl:
                        pendingImageUrl = event->to<API_ShowImageFromUrlEvent>()->url();
                        imagePending = true;
                        break;
                    case EventId::WEB_MqttDisconnected:
                        const auto reason = event->to<WEB_MqttDisconnectedEvent>()->reason();
                        if (reason == AsyncMqttClientDisconnectReason::TCP_DISCONNECTED) {
                            // Disconnected from broker, will auto-retry
                            break;
                        }
                        setScreen("error", 50, "onMqttDisconnect");
                        tft.setTextColor(TFT_RED, TFT_BLACK);
                        tft.setTextSize(2);
                        tft.setCursor(0, 0);
                        tft.println("MQTT disconnected! Code: " + String(static_cast<int>(reason)));
                        break;
                    case EventId::WEB_MqttError:
                        setScreen("error", 30, "onMqttMessage");
                        tft.setTextColor(TFT_RED);
                        tft.setTextSize(2);
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
                    default: ;
                }

                Serial.println(
                    "<<<< EVENT HANDLED. ptr: " + String(reinterpret_cast<int>(event.get())) + "; unique?: " + String(
                        event.unique()));
            }
        }

        if (currentScreen != "clock" && screenTimeout > 0 && millis() - screenSince > screenTimeout) {
            setScreen("clock", 0, "timeout");
        }

        if (sntp_time_was_setup) {
            sntp_time_was_setup = false;
            if (currentScreen == "clock") showClock();
        }

        if (currentScreen == "clock" && millis() - lastClockUpdate > CLOCK_REFRESH_INTERVAL) {
            showClock();
        }

        if (last_keepalive_time > 0 && millis() - last_keepalive_time > KEEPALIVE_TIMEOUT) {
            Serial.println(
                "[AutoSleep] Entering automatic sleep mode due to inactivity. Now: " + String(millis()) +
                ", last keepalive: " + String(last_keepalive_time));
            is_automatically_sleeping = true;
            last_keepalive_time = 0;
            set_tft_brt(0);
        }

        button.tick();
    }
}

// ------------------------
// Arduino setup()
// ------------------------
void setup() {
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    ledcSetup(TFT_BL_PWM_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, TFT_BL_PWM_CHANNEL);
    set_tft_brt(0);

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        setScreen("error", 30, "setup_error");
        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
        tft.println("SPIFFS failed");
        while (true) {
            delay(1000);
        }
    }

    button.begin();
    button.attachClick([] {
        is_manually_sleeping = !is_manually_sleeping;
        is_automatically_sleeping = false;
        set_tft_brt(Settings::instance().brightness);
        last_keepalive_time = 0;
    });

    set_tft_brt(Settings::instance().brightness);

    TJpgDec.setCallback(jpgRenderCallback);
    TJpgDec.setSwapBytes(true);

    configTime(0, 0, "pool.ntp.org");
    // Setup callback for time synchronization
    esp_sntp_set_time_sync_notification_cb([](timeval *) {
        sntp_time_was_setup = true;
        const String timezone = Settings::instance().timezone;
        Serial.printf("Setting Timezone to %s\n", timezone.c_str());
        setenv("TZ", timezone.c_str(), 1);
        tzset();
    });

    // eventQueue = xQueueCreate(20, sizeof(IEvent*)); // Replaced by EventBus

    xTaskCreate(eventHandlerTask, "EventHandlerTask", 4096, nullptr, 2, nullptr);
    xTaskCreate([](void *o) {
        auto s = static_cast<NetworkService *>(o);
        s->run(nullptr);
    }, "NetworkService", 4096, &NetworkService::instance(), 1, nullptr);
    xTaskCreate([](void *o) {
        auto s = static_cast<WeatherService *>(o);
        s->run(nullptr);
    }, "WeatherService", 4096, &WeatherService::instance(), 1, nullptr);
    xTaskCreate([](void *o) {
        auto s = static_cast<WebServer *>(o);
        s->run(nullptr);
    }, "WebServer", 4096, &WebServer::instance(), 1, nullptr);
}

// ------------------------
// Arduino loop()
// ------------------------
void loop() {
    vTaskDelay(portMAX_DELAY);
}
