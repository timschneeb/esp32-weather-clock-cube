//
// Created by tim on 25.09.25.
//

#include "WeatherService.h"

#include <ArduinoJson.h>
#include <WiFi.h>

#include "event/EventBus.h"
#include "event/Events.h"
#include "utils/HTTPRequest.h"
#include "NetworkService.h"
#include "Settings.h"

WeatherService::WeatherService() : Task("WeatherService", 4096, 1) {}

[[noreturn]] void WeatherService::run() {
    for (;;) {
        if (NetworkService::isInApMode() || !NetworkService::isConnected()) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        Serial.println("[WEATHER] Fetch weather started");

        String apiKey = Settings::instance().weatherApiKey;
        String city = Settings::instance().weatherCity;

        if (apiKey.isEmpty() || city.isEmpty()) {
            Serial.println("[WEATHER] No API key or city set");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        char buf[11];
        time_t now = time(nullptr);
        tm* tm_now = localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d", tm_now);
        String todayStr(buf);

        auto skipForecast = todayStr == Settings::instance().weatherDay && !(Settings::instance().weatherTempMin == 0.0 && Settings::instance().weatherTempMax == 0.0);

        httpNowTask.startRequest("http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric");
        if (!skipForecast)
            httpForecastTask.startRequest("http://api.openweathermap.org/data/2.5/forecast?q=" + city + "&appid=" + apiKey + "&units=metric");
        
        while(httpNowTask.isInProgress() || (httpForecastTask.isInProgress() && !skipForecast)){
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        HTTPResult resNow = httpNowTask.result();
        if (resNow.success)
            Serial.println("[WEATHER] (Now) Payload size: " + String(resNow.payload.length()));
        else
            Serial.println("[WEATHER] (Now) Error " + String(resNow.statusCode) + "; payload: " + resNow.payload);

        if (resNow.success) {
            JsonDocument doc;
            if (!deserializeJson(doc, resNow.payload)) {
                Settings::instance().weatherTempDay = doc["main"]["temp"] | 0.0f;
                Settings::instance().weatherHumidity = doc["main"]["humidity"] | 0.0f;
                Settings::instance().weatherIcon = doc["weather"][0]["icon"].as<String>();
            } else {
                Serial.println("[WEATHER] JSON parse error (current)");
            }
        }

        if (!skipForecast) {
            HTTPResult resFc = httpForecastTask.result();
            if (resFc.success)
                Serial.println("[WEATHER] (FC) Payload size: " + String(resFc.payload.length()));
            else
                Serial.println("[WEATHER] (FC) Error " + String(resFc.statusCode) + "; payload: " + resFc.payload);

            if (resFc.success) {
                JsonDocument doc;
                if (!deserializeJson(doc, resFc.payload)) {
                    float minTemp = 999.0f, maxTemp = -999.0f;

                    for (JsonObject entry : doc["list"].as<JsonArray>()) {
                        auto dt_txt = entry["dt_txt"].as<String>();
                        if (dt_txt.startsWith(todayStr)) {
                            float tempMin = entry["main"]["temp_min"] | 0.0f;
                            float tempMax = entry["main"]["temp_max"] | 0.0f;
                            if (tempMin < minTemp) minTemp = tempMin;
                            if (tempMax > maxTemp) maxTemp = tempMax;
                        }
                    }

                    if (minTemp >= 999.0f || maxTemp <= -999.0f) {
                        Serial.println("[WEATHER] No forecasts found for today (no date match)");
                        minTemp = 0.0f;
                        maxTemp = 0.0f;
                    }

                    Settings::instance().weatherTempMin = minTemp;
                    Settings::instance().weatherTempMax = maxTemp;
                } else {
                    Serial.println("[WEATHER] JSON parse error (forecast)");
                }
            }
        }
        else {
            Serial.println("[WEATHER] Forecast fetch skipped, already done for today");
        }

        Settings::instance().save();
        Serial.println("[WEATHER] Weather update complete");
        EventBus::instance().publish<WEA_ForecastUpdatedEvent>();

        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}
