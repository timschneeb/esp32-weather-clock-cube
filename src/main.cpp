#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <TJpg_Decoder.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <FS.h>
#include <SPIFFS.h>
#include <time.h>
#include <esp_sntp.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <vector>
#include <algorithm>

#include "Button.h"

// ------------------------
//  Globals & Config
// ------------------------
Preferences preferences;

// Backlight configuration
#define TFT_BL 14
#define LCD_BL_PWM_CHANNEL 0

// Constants
const char* MQTT_TOPIC = "frigate/reviews";
const char* CLIENT_ID = "ESP32Client";
const char* DEFAULT_SSID = "ESP32_AP";
const char* DEFAULT_PASSWORD = "admin1234";
const unsigned long WIFI_TIMEOUT = 10000;
const unsigned long KEEPALIVE_TIMEOUT = 5 * 60 * 1000;
const int DEBOUNCE_MS = 250;
const unsigned long CLOCK_REFRESH_INTERVAL = 1000UL; // 1 second
const char* daysShort[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* months[] = {"Jan.", "Feb.", "March", "April", "May", "June", "July", "August", "Sept.", "Oct.", "Nov.", "Dec."};
// Variables
String mqttUser = "";
String mqttPass = "";
String frigateIP = "";
String weatherIcon = "";
String lastDrawnWeatherIcon = "";
String lastDate = "";
String weatherCity = "";
String weatherApiKey = "";
String mode = "alert"; // Default to "alert"
String currentScreen = "clock"; // ["clock", "event", "status", "error"]
TFT_eSPI tft = TFT_eSPI();
Button button = Button();
AsyncWebServer server(80);
AsyncMqttClient mqttClient;
String mqttServer = "";
int mqttPort = 1883;
int frigatePort = 5000;
int displayDuration = 30;
int maxImages = 30;
unsigned long lastClockUpdate = 0;
unsigned long lastKeyTime = 0;
unsigned long screenTimeout = 0;
unsigned long screenSince = 0;
boolean sntp_time_was_setup = false;
unsigned long last_keepalive_time = 0;
boolean is_manually_sleeping = false;
boolean is_automatically_sleeping = false;
// --- WEATHER ---
String weatherTempDay = "";
float weatherHumidity = 0.0;
float weatherTemp = 0.0;
float weatherTempMin = 0.0;
float weatherTempMax = 0.0;
unsigned long lastWeatherFetch = 0;
const unsigned long WEATHER_REFRESH_INTERVAL = 15UL * 60UL * 1000UL; // 10 minutes
// --- Slideshow ---
String pendingImageUrl = "";
bool imagePending = false;
String pendingZone = "";
bool slideshowActive = false;
unsigned long slideshowStart = 0;
int currentSlideshowIdx = 0;
unsigned long slideshowInterval = 3000; // Interval in ms
std::vector<String> jpgQueue; // Array of full paths to JPG files
std::vector<unsigned long> eventCallTimes;
unsigned long lastEventCall = 0;

// ------------------------
//  Forward Declarations
// ------------------------
void setScreen(const String& newScreen, unsigned long timeoutSec = 0, const char* by = "");

// ------------------------
//  Backlight
// ------------------------
void set_tft_brt(int brt) {
  // prevent setting brightness when sleeping
  if (is_manually_sleeping || is_automatically_sleeping)
    brt = 0;

  brt = constrain(brt, 0, 255);

  ledcWrite(LCD_BL_PWM_CHANNEL, 255 - brt);
  // Serial.printf("[BACKLIGHT] Brightness set to: %d (inverted: %d)\n", brt, 255 - brt);
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
  if (now - slideshowStart >= displayDuration * 1000UL) {
    Serial.println("[SLIDESHOW] Display duration expired, stopping slideshow");
    slideshowActive = false;
    jpgQueue.clear();
    eventCallTimes.clear();
    setScreen("clock", 0, "slideshow timeout");
    return;
  }

  // Check if it's time for the next image
  if (now - slideshowStart >= currentSlideshowIdx * slideshowInterval) {
    String filename = jpgQueue[currentSlideshowIdx % jpgQueue.size()];
    if (SPIFFS.exists(filename)) {
      File file = SPIFFS.open(filename, FILE_READ);
      if (file) {
        uint32_t fileSize = file.size();
        uint8_t* jpgData = (uint8_t*)malloc(fileSize);
        if (jpgData) {
          size_t bytesRead = file.readBytes((char*)jpgData, fileSize);
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
void setScreen(const String& newScreen, unsigned long timeoutSec, const char* by) {
  Serial.printf("setScreen: from %s to %s (timeout: %lu sec) by: %s\n", currentScreen.c_str(), newScreen.c_str(), timeoutSec, by);

  // Remove old event calls outside displayDuration
  unsigned long now = millis();
  eventCallTimes.erase(
    std::remove_if(eventCallTimes.begin(), eventCallTimes.end(),
      [now](unsigned long t) { return now - t > displayDuration * 1000UL; }),
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
          uint8_t* jpgData = (uint8_t*)malloc(fileSize);
          if (jpgData) {
            size_t bytesRead = file.readBytes((char*)jpgData, fileSize);
            file.close();
            if (bytesRead == fileSize) {
              tft.fillScreen(TFT_BLACK);
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
  Serial.print("Search icon: "); Serial.println(path);
  int iconWidth = 90;
  int iconHeight = 90;
  int x = 240 - iconWidth - 8;
  int y = 240 - iconHeight - 8;
  if (SPIFFS.exists(path)) {
    TJpgDec.drawJpg(x, y, path.c_str());
    Serial.print("[WEATHER] Icon drawn: "); Serial.println(path);
  } else {
    Serial.print("[WEATHER] Icon NOT found: "); Serial.println(path);
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
  String tempValue = String(weatherTemp, 1);
  String tempUnit = "÷c";
  String humidityValue = String((int)weatherHumidity); // Geheel getal voor luchtvochtigheid
  String humidityUnit = "%";
  String tempMinValue = String(weatherTempMin, 1);
  String tempMinUnit = "÷c";
  String tempMaxValue = String(weatherTempMax, 1);
  String tempMaxUnit = "÷c";

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
void displayImageFromAPI(String url, String zone) {
  const int maxTries = 5;
  int tries = 0;
  bool success = false;
  const size_t MAX_FILE_SIZE = 40 * 1024;

  // Construct detectionId from URL
  String detectionId = url.substring(url.lastIndexOf("/events/") + 8, url.indexOf("/snapshot.jpg"));
  int dashIndex = detectionId.indexOf("-");
  String suffix = (dashIndex > 0) ? detectionId.substring(dashIndex + 1) : detectionId;

  // New filename: <suffix>-<zone>.jpg
  String filename = "/events/" + suffix + "-" + zone + ".jpg";
  if (filename.length() >= 32) {
    filename = "/events/default.jpg";
  }

  // Skip if image already exists
  if (SPIFFS.exists(filename)) {
    Serial.print("[DEBUG] Image already exists: "); Serial.println(filename);
    if (std::find(jpgQueue.begin(), jpgQueue.end(), filename) == jpgQueue.end()) {
      jpgQueue.push_back(filename);
    }
    setScreen("event", displayDuration, "displayImageFromAPI");
    return;
  }

  while (tries < maxTries && !success) {
    Serial.print("[DEBUG] Attempt "); Serial.print(tries + 1); Serial.print("/"); Serial.println(url);
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == 200) {
      uint32_t len = http.getSize();
      if (len > MAX_FILE_SIZE) {
        Serial.println("[ERROR] Image too large: " + String(len) + " bytes");
        setScreen("error", 10, "displayImageFromAPI");
        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
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
        if (jpgCount >= maxImages && !oldestFile.isEmpty()) {
          SPIFFS.remove(oldestFile);
          Serial.println("[DEBUG] Removed: " + oldestFile);
        }
      }

      WiFiClient *stream = http.getStreamPtr();
      uint8_t *jpgData = (uint8_t*)malloc(len);
      if (!jpgData) {
        Serial.println("[ERROR] Memory allocation failed!");
        http.end();
        return;
      }

      if (stream->available()) {
        size_t bytesRead = stream->readBytes((char*)jpgData, len);
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
      http.end();
      tries++;
      delay(2000);
    }
  }

  if (!success) {
    Serial.println("[ERROR] Failed to load image after " + String(maxTries) + " attempts");
    setScreen("error", 10, "displayImageFromAPI");
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.println("Loading failed");
  }
}

// ------------------------
//  Weather fetcher
// ------------------------
void fetchWeather() {
  Serial.println("[WEATHER] fetchWeather() started");

  if (weatherApiKey == "" || weatherCity == "") {
    Serial.println("[WEATHER] No weatherApiKey or city set!");
    return;
  }

  // Determine current day as string
  time_t now = time(nullptr);
  struct tm *tm_now = localtime(&now);
  char dateStr[11];
  strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm_now);
  String todayStr(dateStr);
  Serial.print("[WEATHER] Current date (todayStr): "); Serial.println(todayStr);

  // Fetch current weather data
  HTTPClient http;
  String urlNow = "http://api.openweathermap.org/data/2.5/weather?q=" + weatherCity + "&appid=" + weatherApiKey + "&units=metric";
  Serial.print("[WEATHER] Fetching current weather from: "); Serial.println(urlNow);
  http.begin(urlNow);
  int httpCodeNow = http.GET();

  if (httpCodeNow == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      weatherTemp = doc["main"]["temp"] | 0.0;
      weatherHumidity = doc["main"]["humidity"] | 0.0; // Huidige luchtvochtigheid
      weatherIcon = doc["weather"][0]["icon"].as<String>();

      Serial.print("[WEATHER] Current temperature: "); Serial.println(weatherTemp);
      Serial.print("[WEATHER] Current humidity: "); Serial.println(weatherHumidity);
      Serial.print("[WEATHER] Icon: "); Serial.println(weatherIcon);
    } else {
      Serial.print("[WEATHER] JSON parse error (current): "); Serial.println(error.c_str());
    }
  } else {
    Serial.print("[WEATHER] Error fetching current weather, code: "); Serial.println(httpCodeNow);
  }
  http.end();

  // Only update min/max on a new day, or when not yet set
  if (todayStr == weatherTempDay && !(weatherTempMin == 0.0 && weatherTempMax == 0.0)) {
    Serial.println("[WEATHER] Min/max already fetched for this day, no update needed.");
    return;
  }

  // Fetch min/max for today from forecast
  String urlForecast = "http://api.openweathermap.org/data/2.5/forecast?q=" + weatherCity + "&appid=" + weatherApiKey + "&units=metric&lang=en";
  Serial.print("[WEATHER] Fetching forecast from: "); Serial.println(urlForecast);
  http.begin(urlForecast);
  int httpCodeForecast = http.GET();

  if (httpCodeForecast == 200) {
    String forecastPayload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, forecastPayload);

    if (!error) {
      float minTemp = 99.0;
      float maxTemp = -99.0;
      int matched = 0;

      JsonArray list = doc["list"];
      for (JsonObject entry : list) {
        String dt_txt = entry["dt_txt"].as<String>();
        float tempMin = entry["main"]["temp_min"] | 0.0;
        float tempMax = entry["main"]["temp_max"] | 0.0;

        if (dt_txt.startsWith(todayStr)) {
          matched++;
          Serial.print("[WEATHER] Match found for timestamp: "); Serial.print(dt_txt);
          Serial.print(", temp_min: "); Serial.print(tempMin);
          Serial.print(", temp_max: "); Serial.println(tempMax);
          if (tempMin < minTemp) minTemp = tempMin;
          if (tempMax > maxTemp) maxTemp = tempMax;
        }
      }

      if (matched > 0) {
        weatherTempMin = minTemp;
        weatherTempMax = maxTemp;
        Serial.print("[WEATHER] Minimum temperature today: "); Serial.println(weatherTempMin);
        Serial.print("[WEATHER] Maximum temperature today: "); Serial.println(weatherTempMax);
        if (matched == 1) {
          Serial.println("[WEATHER] Warning: Only one timestamp found for today, min/max based on single data point.");
        }
      } else {
        Serial.println("[WEATHER] No forecasts found for today (no date match).");
        weatherTempMin = 0;
        weatherTempMax = 0;
        Serial.print("[WEATHER] Minimum temperature today: "); Serial.println(weatherTempMin);
        Serial.print("[WEATHER] Maximum temperature today: "); Serial.println(weatherTempMax);
      }
    } else {
      Serial.print("[WEATHER] JSON parse error (forecast): "); Serial.println(error.c_str());
      weatherTempMin = 0;
      weatherTempMax = 0;
      Serial.print("[WEATHER] Minimum temperature today: "); Serial.println(weatherTempMin);
      Serial.print("[WEATHER] Maximum temperature today: "); Serial.println(weatherTempMax);
    }
  } else {
    Serial.print("[WEATHER] Error fetching forecast, code: "); Serial.println(httpCodeForecast);
    weatherTempMin = 0;
    weatherTempMax = 0;
    Serial.print("[WEATHER] Minimum temperature today: "); Serial.println(weatherTempMin);
    Serial.print("[WEATHER] Maximum temperature today: "); Serial.println(weatherTempMax);
  }

  // Set the day and save all values
  weatherTempDay = todayStr;
  preferences.putString("day", weatherTempDay);
  preferences.putFloat("min", weatherTempMin);
  preferences.putFloat("max", weatherTempMax);
  preferences.putFloat("humidity", weatherHumidity); // Sla luchtvochtigheid op

  http.end();
}

// ------------------------
//  MQTT callbacks
// ------------------------
void onMqttConnect(bool sessionPresent) {
  Serial.println("[MQTT] Connected!");
  mqttClient.subscribe(MQTT_TOPIC, 0);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  static unsigned long lastReconnectAttempt = 0;
  const unsigned long reconnectInterval = 50000;
  Serial.print("[MQTT] Connection lost with reason: "); Serial.println(static_cast<int>(reason));
  unsigned long now = millis();
  if (now - lastReconnectAttempt >= reconnectInterval) {
    setScreen("error", 50, "onMqttDisconnect");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println("MQTT ERROR!");
    Serial.println("[MQTT] Attempting to reconnect...");
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCredentials(mqttUser.c_str(), mqttPass.c_str());
    mqttClient.connect();
    lastReconnectAttempt = now;
  } else {
    Serial.print("[MQTT] Waiting for next reconnect attempt in "); 
    Serial.print((reconnectInterval - (now - lastReconnectAttempt)) / 1000);
    Serial.println(" seconds");
  }
}

// ------------------------
//  MQTT message handler
// ------------------------
void onMqttMessage(
    char* topic,
    char* payload,
    AsyncMqttClientMessageProperties properties,
    size_t len,
    size_t index,
    size_t total
) {
  String payloadStr;
  for (size_t i = 0; i < len; i++) payloadStr += (char)payload[i];
  Serial.println("====[MQTT RECEIVED]====");
  Serial.print("Topic:   "); Serial.println(topic);
  Serial.print("Payload: "); Serial.println(payloadStr);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, len);
  if (error) {
    Serial.print("[DEBUG] JSON parsing error: "); Serial.println(error.c_str());
    setScreen("error", 30, "onMqttMessage");
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.println("JSON Parse Error");
    return;
  }

  String type = doc["type"] | "";
  JsonObject msg;
  String severity = "";

  if (type == "new" && doc["before"].is<JsonObject>()) {
    msg = doc["before"].as<JsonObject>();
  } else if (type == "update" && doc["after"].is<JsonObject>()) {
    msg = doc["after"].as<JsonObject>();
  } else {
    Serial.println("[DEBUG] Ignoring message of type '" + type + "'");
    return;
  }

  if (msg["severity"].is<String>()) severity = msg["severity"].as<String>();

  String modeClean = mode; modeClean.trim(); modeClean.toLowerCase();
  String severityClean = severity; severityClean.trim(); severityClean.toLowerCase();
  bool show = modeClean.indexOf(severityClean) >= 0;

  if (show && frigateIP.length() > 0 && msg["data"].is<JsonObject>() && msg["data"]["detections"].is<JsonArray>()) {
    JsonArray detections = msg["data"]["detections"].as<JsonArray>();
    JsonArray zonesArray = msg["data"]["zones"].is<JsonArray>() ? msg["data"]["zones"].as<JsonArray>() : JsonArray();
    String zone = "outside-zone";
    if (!zonesArray.isNull() && zonesArray.size() > 0) {
      zone = String(zonesArray[zonesArray.size() - 1].as<const char*>());
    }
  
    if (!detections.isNull() && detections.size() > 0) {
      for (JsonVariant d : detections) {
        String id = d.as<String>();
        int dashIndex = id.indexOf("-");
        String suffix = (dashIndex > 0) ? id.substring(dashIndex + 1) : id;
        String filename = "/events/" + suffix + "-" + zone + ".jpg";
        if (std::find(jpgQueue.begin(), jpgQueue.end(), filename) == jpgQueue.end()) {
          jpgQueue.push_back(filename);
        }
      }
      String url = "http://" + frigateIP + ":" + String(frigatePort) +
                   "/api/events/" + detections[0].as<String>() + "/snapshot.jpg?crop=1&height=240";
      imagePending = true;
      pendingImageUrl = url;
      pendingZone = zone;
    }
  }
}


// ------------------------
//  Preferences Config Struct Helper
// ------------------------
struct Config {
  String ssid;
  String pwd;
  String mqtt;
  int mqttPort;
  String mqttUser;
  String mqttPass;
  String fip;
  int fport;
  int sec;
  String mode;
  String weatherApiKey;
  String weatherCity;
  int maxImages;
  int brightness;
  int autoBrtStartHr;
  int autoBrtEndHr;
  int autoBrtLvl;
  bool autoBrtEnabled;
  int timezone;
};

Config getConfig() {
  Config config;
  preferences.begin("config", true);
  config.ssid = preferences.getString("ssid", "");
  config.pwd = preferences.getString("pwd", "");
  config.mqtt = preferences.getString("mqtt", "");
  config.mqttPort = preferences.getInt("mqttport", 1883);
  config.mqttUser = preferences.getString("mqttuser", "");
  config.mqttPass = preferences.getString("mqttpass", "");
  config.fip = preferences.getString("fip", "");
  config.fport = preferences.getInt("fport", 5000);
  config.sec = preferences.getInt("sec", 30);
  config.mode = preferences.getString("mode", "alert");
  config.weatherApiKey = preferences.getString("weatherApiKey", "");
  config.weatherCity = preferences.getString("weatherCity", "");
  config.maxImages = preferences.getInt("maxImages", 10);
  config.brightness = preferences.getInt("brightness", 100);
  config.autoBrtStartHr = preferences.getInt("autoBrtStartHr", 0);
  config.autoBrtEndHr = preferences.getInt("autoBrtEndHr", 0);
  config.autoBrtLvl = preferences.getInt("autoBrtLvl", 0);
  config.autoBrtEnabled = preferences.getBool("autoBrtEnabled", false);
  config.timezone = preferences.getInt("timezone", 0);
  preferences.end();
  return config;
}

void AutoBrightnessSchedule() {
  Config cfg = getConfig();
  Serial.printf("[AUTO-BRT] autoBrtEnabled: %d, autoBrtLvl: %d, brightness: %d\n", cfg.autoBrtEnabled, cfg.autoBrtLvl, cfg.brightness);
  if (!cfg.autoBrtEnabled) {
    Serial.println("[AUTO-BRT] Automatic brightness disabled, setting to brightness");
    set_tft_brt(cfg.brightness);
    return;
  }

  time_t now = time(nullptr);
  struct tm *tm_info = localtime(&now);
  if (!tm_info) {
    Serial.println("[AUTO-BRT] Time not synchronized!");
    return;
  }

  int currentHour = tm_info->tm_hour;
  bool inRange = false;

  if (cfg.autoBrtStartHr < cfg.autoBrtEndHr) {
    inRange = (currentHour >= cfg.autoBrtStartHr && currentHour < cfg.autoBrtEndHr);
  } else {
    inRange = (currentHour >= cfg.autoBrtStartHr || currentHour < cfg.autoBrtEndHr);
  }
  Serial.printf("[AUTO-BRT] Current hour: %d, inRange: %d (start: %d, end: %d)\n", currentHour, inRange, cfg.autoBrtStartHr, cfg.autoBrtEndHr);

  int newBrt = inRange ? cfg.autoBrtLvl : cfg.brightness;
  set_tft_brt(newBrt);
  Serial.printf("[AUTO-BRT] Applied brightness: %d (hour=%d)\n", newBrt, currentHour);

  static bool lastInRange = false;
  lastInRange = inRange;
}

// ------------------------
//  SPIFFS images list helper
// ------------------------
String getImagesList() {
  String html = "<ul class='image-list'>";
  File root = SPIFFS.open("/events");
  
  if (!root || !root.isDirectory()) {
    html += "<li>Could not open SPIFFS</li>";
    html += "</ul>";
    return html;
  }

  struct FileInfo {
    String name;
    String displayName;
    unsigned long mtime;
  };
  std::vector<FileInfo> files;

  File file = root.openNextFile();
  while (file) {
    String fname = file.name();
    if (fname.endsWith(".jpg")) {
      String displayName = fname;
      if (!fname.startsWith("/")) {
        fname = "/events/" + fname;
      }
      FileInfo info;
      info.name = fname;
      info.displayName = displayName;
      info.mtime = file.getLastWrite();
      files.push_back(info);
    }
    file = root.openNextFile();
  }
  root.close();

  std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
    return a.mtime > b.mtime;
  });

  for (const FileInfo& info : files) {
    html += "<li>";
    html += "<img src='" + info.name + "' alt='Event image'>";
    html += "<a href='" + info.name + "'>" + info.displayName + "</a></li>";
  }

  html += "</ul>";
  return html;
}

// ------------------------
//  Helper for checked attr
// ------------------------
String getCheckedAttribute(bool isChecked) {
  return isChecked ? "checked" : "";
}

// ------------------------
//  Webinterface
// ------------------------
void setupWebInterface() {
  server.serveStatic("/styles.css", SPIFFS, "/styles.css");
  server.serveStatic("/scripts.js", SPIFFS, "/scripts.js");
  server.serveStatic("/events", SPIFFS, "/events");
  server.serveStatic("/icons", SPIFFS, "/icons");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Config config = getConfig();
    String modeLower = config.mode;
    modeLower.toLowerCase();
    bool isAlertChecked = modeLower.indexOf("alert") >= 0;
    bool isDetectionChecked = modeLower.indexOf("detection") >= 0;

    String alertCheckbox = "<input type='checkbox' id='mode_alert' name='mode_alert' value='alert' " + getCheckedAttribute(isAlertChecked) + ">";
    String detectionCheckbox = "<input type='checkbox' id='mode_detection' name='mode_detection' value='detection' " + getCheckedAttribute(isDetectionChecked) + ">";

    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      request->send(500, "text/plain", "Could not open index.html");
      return;
    }
    String html = file.readString();
    file.close();

    // Fill HTML placeholders
    html.replace("{{ssid}}", config.ssid);
    html.replace("{{pwd}}", config.pwd != "" ? "******" : "");
    html.replace("{{pwd_exists}}", config.pwd != "" ? "1" : "0");
    html.replace("{{mqtt}}", config.mqtt);
    html.replace("{{mqttport}}", String(config.mqttPort));
    html.replace("{{mqttuser}}", config.mqttUser);
    html.replace("{{mqttpass}}", config.mqttPass != "" ? "******" : "");
    html.replace("{{mqttpass_exists}}", config.mqttPass != "" ? "1" : "0");
    html.replace("{{fip}}", config.fip);
    html.replace("{{fport}}", String(config.fport));
    html.replace("{{sec}}", String(config.sec));
    html.replace("{{maxImages}}", String(config.maxImages));
    html.replace("{{slideshowInterval}}", String(slideshowInterval));
    html.replace("{{alertCheckbox}}", alertCheckbox);
    html.replace("{{detectionCheckbox}}", detectionCheckbox);
    html.replace("{{weatherApiKey}}", config.weatherApiKey != "" ? "******" : "");
    html.replace("{{weatherApiKey_exists}}", config.weatherApiKey != "" ? "1" : "0");
    html.replace("{{weatherCity}}", config.weatherCity);
    html.replace("{{timezone}}", String(config.timezone));
    html.replace("{{brightness}}", String(config.brightness));
    html.replace("{{autoBrtLvl}}", String(config.autoBrtLvl));
    html.replace("{{autoBrtEnabled}}", getCheckedAttribute(config.autoBrtEnabled));
    html.replace("{{autoBrtStartHr}}", String(config.autoBrtStartHr));
    html.replace("{{autoBrtEndHr}}", String(config.autoBrtEndHr));
    html.replace("{{totalBytes}}", String(SPIFFS.totalBytes() / 1024));
    html.replace("{{usedBytes}}", String(SPIFFS.usedBytes() / 1024));
    html.replace("{{freeBytes}}", String((SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024));
    html.replace("{{imagesList}}", getImagesList());

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Safe integer fetch helper
    auto getIntParam = [](AsyncWebServerRequest* req, const char* name, int def) -> int {
      if (req->getParam(name, true)) {
        String v = req->getParam(name, true)->value();
        if (v.length() > 0) return v.toInt();
      }
      return def;
    };
  
    preferences.begin("config", false);
    String currentPwd = preferences.getString("pwd", "");
    String currentMqttPass = preferences.getString("mqttpass", "");
    String currentApiKey = preferences.getString("weatherApiKey", "");
  
    String newSSID = request->getParam("ssid", true)->value();
    preferences.putString("ssid", newSSID);
  
    String newPwd = request->getParam("pwd", true)->value();
    bool pwdExists = request->getParam("pwd_exists", true)->value() == "1";
    if (!pwdExists || newPwd != "******") {
      preferences.putString("pwd", newPwd);
    }
  
    String newMqtt = request->getParam("mqtt", true)->value();
    preferences.putString("mqtt", newMqtt);
  
    int newMqttPort = getIntParam(request, "mqttport", 0);
    if (newMqttPort < 1 || newMqttPort > 65535) newMqttPort = 1883;
    preferences.putInt("mqttport", newMqttPort);
  
    String newMqttUser = request->getParam("mqttuser", true)->value();
    preferences.putString("mqttuser", newMqttUser);
  
    String newMqttPass = request->getParam("mqttpass", true)->value();
    bool mqttPassExists = request->getParam("mqttpass_exists", true)->value() == "1";
    if (!mqttPassExists || newMqttPass != "******") {
      preferences.putString("mqttpass", newMqttPass);
    }
  
    String newFip = request->getParam("fip", true)->value();
    preferences.putString("fip", newFip);
  
    int newFport = getIntParam(request, "fport", 0);
    if (newFport < 1 || newFport > 65535) newFport = 5000;
    preferences.putInt("fport", newFport);
  
    int newSec = getIntParam(request, "sec", 0);
    if (newSec < 1) newSec = 30;
    preferences.putInt("sec", newSec);
    displayDuration = newSec;
  
    int newMaxImages = getIntParam(request, "maxImages", 0);
    if (newMaxImages < 1) newMaxImages = 1;
    if (newMaxImages > 60) newMaxImages = 60;
    preferences.putInt("maxImages", newMaxImages);
    maxImages = newMaxImages;
  
    int newSlideshowInterval = getIntParam(request, "slideshowInterval", 0);
    if (newSlideshowInterval < 500) newSlideshowInterval = 3000;
    if (newSlideshowInterval > 20000) newSlideshowInterval = 20000;
    slideshowInterval = newSlideshowInterval;
    preferences.putInt("slideInterval", slideshowInterval);
  
    // Modes
    String modeValue = "";
    if (request->hasParam("mode_alert", true)) {
      if (request->getParam("mode_alert", true)->value() == "alert") {
        modeValue += "alert";
      }
    }
    if (request->hasParam("mode_detection", true)) {
      if (request->getParam("mode_detection", true)->value() == "detection") {
        if (modeValue != "") modeValue += ",";
        modeValue += "detection";
      }
    }
    preferences.putString("mode", modeValue);
    mode = modeValue;
  
    // Weather
    String newApiKey = request->getParam("weatherApiKey", true)->value();
    bool apiKeyExists = request->getParam("weatherApiKey_exists", true)->value() == "1";
    if (!apiKeyExists || newApiKey != "******") {
      preferences.putString("weatherApiKey", newApiKey);
      fetchWeather();
    }
    String newCity = request->getParam("weatherCity", true)->value();
    preferences.putString("weatherCity", newCity);
  
    int newTimezone = getIntParam(request, "timezone", 0);
    preferences.putInt("timezone", newTimezone);
  
    // Brightness
    int newBrightness = getIntParam(request, "brightness", 0);
    if (newBrightness >= 0 && newBrightness <= 255) {
      preferences.putInt("brightness", newBrightness);
    }
  
    // Auto-brightness
    int startHour = getIntParam(request, "autoBrtStartHr", 0);
    int endHour   = getIntParam(request, "autoBrtEndHr", 0);
    int brtLevel  = getIntParam(request, "autoBrtLvl", 0);
    bool enabled  = request->hasParam("autoBrtEnabled", true);
  
    preferences.putInt("autoBrtStartHr", constrain(startHour, 0, 23));
    preferences.putInt("autoBrtEndHr", constrain(endHour, 0, 23));
    preferences.putInt("autoBrtLvl", constrain(brtLevel, 0, 255));
    preferences.putBool("autoBrtEnabled", enabled);
  
    preferences.end();
  
    bool mqttConfigChanged = (newMqtt != mqttServer || newMqttPort != mqttPort || newMqttUser != mqttUser || (newMqttPass != "******" && newMqttPass != mqttPass));
    mqttServer = newMqtt;
    mqttPort = newMqttPort;
    mqttUser = newMqttUser;
    mqttPass = newMqttPass;
    frigatePort = newFport;
  
    if (mqttConfigChanged) {
      if (mqttClient.connected()) mqttClient.disconnect();
      mqttClient.setServer(mqttServer.c_str(), mqttPort);
      mqttClient.setCredentials(mqttUser.c_str(), mqttPass.c_str());
      mqttClient.connect();
    }

    AutoBrightnessSchedule();

    String cacheBuster = "/?v=" + String(millis());
    request->redirect(cacheBuster);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    File file = SPIFFS.open("/update.html", "r");
    if (!file) {
      request->send(500, "text/plain", "Could not open update.html");
      return;
    }
    String html = file.readString();
    file.close();
    request->send(200, "text/html", html);
  });

  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool ok = !Update.hasError();
      request->send(200, "text/plain", ok ? "Update completed. Restarting..." : "Update failed!");
      if (ok) {
        delay(1000);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        Update.begin();
      }
      if (!Update.hasError()) {
        Update.write(data, len);
      }
      if (final) {
        Update.end(true);
      }
    });

  server.on("/delete_all", HTTP_GET, [](AsyncWebServerRequest *request) {
    int deleted = 0;
    File root = SPIFFS.open("/events");
    if (!root || !root.isDirectory()) {
      request->send(500, "text/plain", "Could not open /events");
      return;
    }
    File file = root.openNextFile();
    while (file) {
      String fname = file.name();
      if (fname.endsWith(".jpg")) {
        if (!fname.startsWith("/")) {
          fname = "/events/" + fname;
        }
        if (SPIFFS.exists(fname) && SPIFFS.remove(fname)) {
          deleted++;
        }
      }
      file = root.openNextFile();
    }
    root.close();
    String response = "Deleted: " + String(deleted) + " images";
    request->send(200, "text/plain", response);
    String cacheBuster = "/?v=" + String(millis());
    request->redirect(cacheBuster);
  });

  server.on("/show_image", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("url")) {
      String url = request->getParam("url")->value();
      pendingImageUrl = url;
      imagePending = true;
      request->send(200, "text/plain", "Image will be shown on display!");
    } else {
      request->send(400, "text/plain", "Missing url parameter");
    }
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("[WEB] Reboot requested via /reboot");
      request->send(200, "text/plain", "Rebooting ESP32...");
      delay(500);
      ESP.restart();
  });

  server.on("/keepalive", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("[WEB] Keep-alive signal received");
      last_keepalive_time = millis();
      is_automatically_sleeping = false;
      request->send(200, "application/json",
        "{\"now\": " + String(last_keepalive_time) + ", \"alive_until\": " + String(last_keepalive_time+KEEPALIVE_TIMEOUT) + "}");
      set_tft_brt(getConfig().brightness);
  });

  server.begin();
}

// ------------------------
// WiFi connection
// ------------------------
void setupWiFi() {
  preferences.begin("config", false);
  String ssid = preferences.getString("ssid", "");
  String pwd = preferences.getString("pwd", "");
  preferences.end();

  bool fallbackAP = false;

  if (ssid == "") {
    Serial.println("No saved SSID, switching to AP mode...");
    fallbackAP = true;
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pwd.c_str());
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < WIFI_TIMEOUT) {
      delay(500);
      Serial.println("Connecting to WiFi...");
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi failed, fallback to AP mode...");
      fallbackAP = true;
    } else {
      Serial.println("WiFi connected to: " + ssid);
      Serial.println("IP address: " + WiFi.localIP().toString());
      setScreen("statusWiFi", 5, "show_wifi_status");
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(2);
      tft.setCursor(10, 40);
      tft.println("Connected to:");
      tft.setCursor(10, 70);
      tft.println(ssid);
      tft.setCursor(10, 110);
      tft.println("IP:");
      tft.setCursor(10, 140);
      tft.println(WiFi.localIP());
    }
  }

  if (fallbackAP) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(DEFAULT_SSID, DEFAULT_PASSWORD);
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
    tft.println("IP: 192.168.4.1");
  }

  setupWebInterface();
}

// ------------------------
// Arduino setup()
// ------------------------
void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    setScreen("error", 30, "setup_error");
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.println("SPIFFS failed");
    while (true) delay(1000);
  }

  ledcSetup(LCD_BL_PWM_CHANNEL, 5000, 8);
  ledcAttachPin(TFT_BL, LCD_BL_PWM_CHANNEL);

  button.begin();
  button.attachClick([] {
    is_manually_sleeping = !is_manually_sleeping;
    is_automatically_sleeping = false;
    set_tft_brt(getConfig().brightness);
    last_keepalive_time = 0;
  });

  tft.begin();
  tft.setRotation(0);

  preferences.begin("config", false);
  int brightness = getConfig().brightness;
  preferences.end();

  tft.fillScreen(TFT_BLACK);

  TJpgDec.setCallback(jpgRenderCallback);
  TJpgDec.setSwapBytes(true);

  preferences.begin("config", false);
  mqttServer = preferences.getString("mqtt", "");
  mqttPort = preferences.getInt("mqttport", 1883);
  mqttUser = preferences.getString("mqttuser", "");
  mqttPass = preferences.getString("mqttpass", "");
  frigateIP = preferences.getString("fip", "");
  frigatePort = preferences.getInt("fport", 5000);
  displayDuration = preferences.getInt("sec", 30);
  mode = preferences.getString("mode", "alert");
  weatherApiKey = preferences.getString("weatherApiKey", "");
  weatherCity = preferences.getString("weatherCity", "");
  weatherHumidity = preferences.getFloat("humidity", 0.0);
  weatherTempDay = preferences.getString("day", "");
  weatherTempMin = preferences.getFloat("min", 0.0);
  weatherTempMax = preferences.getFloat("max", 0.0);
  int timezoneVal = preferences.getInt("timezone", 0);
  maxImages = preferences.getInt("maxImages", 30);
  slideshowInterval = preferences.getInt("slideInterval", 3000);
  preferences.end();

  long gmtOffset_sec = timezoneVal * 3600L;
  Serial.printf("configTime: gmtOffset_sec = %ld\n", gmtOffset_sec);
  configTime(gmtOffset_sec, 0, "pool.ntp.org");
  // Setup callback for time synchronization
  esp_sntp_set_time_sync_notification_cb([](timeval*) {
    sntp_time_was_setup = true;
  });

  setupWiFi();

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(mqttServer.c_str(), mqttPort);
  mqttClient.setCredentials(mqttUser.c_str(), mqttPass.c_str());
  if (WiFi.status() == WL_CONNECTED) mqttClient.connect();

  fetchWeather();
  lastWeatherFetch = millis();
  AutoBrightnessSchedule();
}

// ------------------------
// Arduino loop()
// ------------------------
void loop() {
  if (imagePending) {
    imagePending = false;
    displayImageFromAPI(pendingImageUrl, pendingZone);
  }

  static wl_status_t lastStatus = WL_CONNECTED;
  static unsigned long lastReconnectAttempt = 0;
  static unsigned long lastAutoBrtCheck = 0;
  const unsigned long RECONNECT_INTERVAL = 5000;
  const unsigned long AUTO_BRT_INTERVAL = 60000; // 60 seconds interval

  if (WiFi.status() == WL_CONNECTED && lastStatus != WL_CONNECTED && millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
    mqttClient.connect();
    lastReconnectAttempt = millis();
  }
  lastStatus = WiFi.status();

  if (slideshowActive) {
    handleSlideshow();
  }

  if (currentScreen != "clock" && screenTimeout > 0 && millis() - screenSince > screenTimeout) {
    setScreen("clock", 0, "timeout");
  }

  if (millis() - lastWeatherFetch > WEATHER_REFRESH_INTERVAL || sntp_time_was_setup) {
    sntp_time_was_setup = false;

    lastWeatherFetch = millis();
    fetchWeather();
    if (currentScreen == "clock") showClock();
  }

  if (millis() - lastAutoBrtCheck > AUTO_BRT_INTERVAL) {
    AutoBrightnessSchedule();
    lastAutoBrtCheck = millis();
  }

  if (currentScreen == "clock" && millis() - lastClockUpdate > CLOCK_REFRESH_INTERVAL) {
    showClock();
  }

  if (last_keepalive_time > 0 && millis() - last_keepalive_time > KEEPALIVE_TIMEOUT) {
    Serial.println("[AutoSleep] Entering automatic sleep mode due to inactivity.");
    is_automatically_sleeping = true;
    last_keepalive_time = 0;
    set_tft_brt(0);
  }

  button.tick();
}