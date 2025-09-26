//
// Created by tim on 25.09.25.
//

#include "WeatherService.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <QuarkTS.h>
#include <WiFi.h>

#include "Events.h"
#include "HTTPRequest.h"
#include "Settings.h"
#include "TFT_eSPI.h"

WeatherService::WeatherService() = default;

void WeatherService::registerTask() {
    qOS::os.add(instance(), nullptr, qOS::core::MEDIUM_PRIORITY, 20000, PERIODIC);
    qOS::os.notify(qOS::notifyMode::SIMPLE, instance(), nullptr);
    instance().setName("Weather");
}

void WeatherService::activities(qOS::event_t e) {
    qOS::co::reenter() {
        Serial.println(">>>>> [WEATHER] Service running: " + String(static_cast<int>(e.getTrigger())));

        Serial.println("[WEATHER] Fetch weather started");

        String apiKey = Settings::instance().weatherApiKey;
        String city = Settings::instance().weatherCity;

        if (apiKey.isEmpty() || city.isEmpty()) {
            Serial.println("[WEATHER] No API key or city set");
            return;
        }

        char buf[11];
        time_t now = ::time(nullptr);
        tm* tm_now = localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d", tm_now);
        String todayStr(buf);

        auto skipForecast = todayStr == Settings::instance().weatherDay && !(Settings::instance().weatherTempMin == 0.0 && Settings::instance().weatherTempMax == 0.0);

        httpNowTask.startRequest("http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric");
        if (!skipForecast)
            httpForecastTask.startRequest("http://api.openweathermap.org/data/2.5/forecast?q=" + city + "&appid=" + apiKey + "&units=metric");
        qOS::co::waitUntil(!httpNowTask.isInProgress() && (!httpForecastTask.isInProgress() || skipForecast));

        HTTPResult resNow = httpNowTask.result();
        if (resNow.success)
            Serial.println("[WEATHER] (Now) Payload: " + resNow.payload);
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
                Serial.println("[WEATHER] (FC) Payload: " + resFc.payload);
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
        qOS::os.notify(qOS::notifyMode::QUEUED, new WEA_ForecastUpdatedEvent());

        task::activities(e);

        qOS::co::restart();
        /*Serial.println("[WEATHER] fetchWeather() started");

        String weatherApiKey = Settings::instance().weatherApiKey;
        String weatherCity = Settings::instance().weatherCity;

        if (weatherApiKey == "" || weatherCity == "") {
            Serial.println("[WEATHER] No weatherApiKey or city set!");
            return;
        }

        // Determine current day as string
        time_t now = ::time(nullptr);
        tm *tm_now = localtime(&now);
        char dateStr[11];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tm_now);
        String todayStr(dateStr);
        Serial.print("[WEATHER] Current date (todayStr): ");
        Serial.println(todayStr);

        // Fetch current weather data
        HTTPClient http;
        String urlNow = "http://api.openweathermap.org/data/2.5/weather?q=" + weatherCity + "&appid=" + weatherApiKey +
                        "&units=metric";
        Serial.print("[WEATHER] Fetching current weather from: ");
        Serial.println(urlNow);
        http.begin(urlNow);
        int httpCodeNow = http.GET();

        String weatherIcon = Settings::instance().weatherIcon;
        String weatherTempDay = Settings::instance().weatherTempDay;
        float weatherTempMin = Settings::instance().weatherTempMin;
        float weatherTempMax = Settings::instance().weatherTempMax;
        float weatherHumidity = Settings::instance().weatherHumidity;

        if (httpCodeNow == 200) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                weatherTempDay = doc["main"]["temp"] | 0.0f;
                weatherHumidity = doc["main"]["humidity"] | 0.0f; // Huidige luchtvochtigheid
                weatherIcon = doc["weather"][0]["icon"].as<String>();

                Serial.print("[WEATHER] Current temperature: ");
                Serial.println(weatherTempDay);
                Serial.print("[WEATHER] Current humidity: ");
                Serial.println(weatherHumidity);
                Serial.print("[WEATHER] Icon: ");
                Serial.println(weatherIcon);
            } else {
                Serial.print("[WEATHER] JSON parse error (current): ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.print("[WEATHER] Error fetching current weather, code: ");
            Serial.println(httpCodeNow);
        }
        http.end();

        // Only update min/max on a new day, or when not yet set
        if (todayStr == weatherTempDay && !(weatherTempMin == 0.0 && weatherTempMax == 0.0)) {
            Serial.println("[WEATHER] Min/max already fetched for this day, no update needed.");
            return;
        }

        // Fetch min/max for today from forecast
        String urlForecast = "http://api.openweathermap.org/data/2.5/forecast?q=" + weatherCity + "&appid=" + weatherApiKey
                             + "&units=metric&lang=en";
        Serial.print("[WEATHER] Fetching forecast from: ");
        Serial.println(urlForecast);
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
                for (JsonObject entry: list) {
                    auto dt_txt = entry["dt_txt"].as<String>();
                    float tempMin = entry["main"]["temp_min"] | 0.0f;
                    float tempMax = entry["main"]["temp_max"] | 0.0f;

                    if (dt_txt.startsWith(todayStr)) {
                        matched++;
                        Serial.print("[WEATHER] Match found for timestamp: ");
                        Serial.print(dt_txt);
                        Serial.print(", temp_min: ");
                        Serial.print(tempMin);
                        Serial.print(", temp_max: ");
                        Serial.println(tempMax);
                        if (tempMin < minTemp) minTemp = tempMin;
                        if (tempMax > maxTemp) maxTemp = tempMax;
                    }
                }

                if (matched > 0) {
                    weatherTempMin = minTemp;
                    weatherTempMax = maxTemp;
                    Serial.print("[WEATHER] Minimum temperature today: ");
                    Serial.println(weatherTempMin);
                    Serial.print("[WEATHER] Maximum temperature today: ");
                    Serial.println(weatherTempMax);
                    if (matched == 1) {
                        Serial.println(
                            "[WEATHER] Warning: Only one timestamp found for today, min/max based on single data point.");
                    }
                } else {
                    Serial.println("[WEATHER] No forecasts found for today (no date match).");
                    weatherTempMin = 0;
                    weatherTempMax = 0;
                    Serial.print("[WEATHER] Minimum temperature today: ");
                    Serial.println(weatherTempMin);
                    Serial.print("[WEATHER] Maximum temperature today: ");
                    Serial.println(weatherTempMax);
                }
            } else {
                Serial.print("[WEATHER] JSON parse error (forecast): ");
                Serial.println(error.c_str());
                weatherTempMin = 0;
                weatherTempMax = 0;
                Serial.print("[WEATHER] Minimum temperature today: ");
                Serial.println(weatherTempMin);
                Serial.print("[WEATHER] Maximum temperature today: ");
                Serial.println(weatherTempMax);
            }
        } else {
            Serial.print("[WEATHER] Error fetching forecast, code: ");
            Serial.println(httpCodeForecast);
            weatherTempMin = 0;
            weatherTempMax = 0;
            Serial.print("[WEATHER] Minimum temperature today: ");
            Serial.println(weatherTempMin);
            Serial.print("[WEATHER] Maximum temperature today: ");
            Serial.println(weatherTempMax);
        }

        // Set the day and save all values
        Settings::instance().weatherIcon = weatherIcon;
        Settings::instance().weatherTempDay = weatherTempDay;
        Settings::instance().weatherTempMin = weatherTempMin;
        Settings::instance().weatherTempMax = weatherTempMax;
        Settings::instance().weatherHumidity = weatherHumidity;
        Settings::instance().save();

        http.end();

        task::activities(e);*/
    }
}
