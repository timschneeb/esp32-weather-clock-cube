//
// Created by tim on 25.09.25.
//

#include "WeatherService.h"

#include <ArduinoJson.h>

#include <algorithm>

#include "event/EventBus.h"
#include "event/Events.h"
#include "utils/HTTPRequest.h"
#include "services/NetworkService.h"
#include "Settings.h"
#include "utils/Macros.h"

WeatherService::WeatherService() : Task("WeatherService", 4096, 1) {}

bool WeatherService::findMinMaxForecast(JsonDocument doc, const String &date, float &minTemp, float &maxTemp) {
    minTemp = 999.0f;
    maxTemp = -999.0f;

    for (JsonObject entry : doc["list"].as<JsonArray>()) {
        auto dt_txt = entry["dt_txt"].as<String>();
        if (dt_txt.startsWith(date)) {
            minTemp = std::min(entry["main"]["temp_min"] | 0.0f, minTemp);
            maxTemp = std::max(entry["main"]["temp_max"] | 0.0f, maxTemp);
        }
    }
    return minTemp < 999.0f && maxTemp > -999.0f;
}

String WeatherService::currentLocalDate(const int dayOffset) {
    char buf[11];
    const time_t now = time(nullptr);
    tm* tm_now = localtime(&now);
    if (dayOffset != 0) {
        tm_now->tm_mday += dayOffset;
        mktime(tm_now);
    }
    strftime(buf, sizeof(buf), "%Y-%m-%d", tm_now);
    return String(buf);
}

[[noreturn]] void WeatherService::run() {
    for (;;) {
        if (NetworkService::isInApMode() || !NetworkService::isConnected()) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        String apiKey = Settings::instance().weatherApiKey;
        String city = Settings::instance().weatherCity;

        if (apiKey.isEmpty() || city.isEmpty()) {
            // No API key or city configured, no action.
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        LOG_INFO("Fetch weather started");

        String todayDate = currentLocalDate(0);
        String tomorrowDate = currentLocalDate(1);
        auto skipForecast = todayDate == Settings::instance().weatherDay && (Settings::instance().weatherTempMin != 0.0 || Settings::instance().weatherTempMax != 0.0);

        httpNowTask.startRequest("http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric");
        if (!skipForecast)
            httpForecastTask.startRequest("http://api.openweathermap.org/data/2.5/forecast?q=" + city + "&appid=" + apiKey + "&units=metric");
        
        while(httpNowTask.isInProgress() || (httpForecastTask.isInProgress() && !skipForecast)){
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        HTTPResult resNow = httpNowTask.result();
        if (resNow.success) {
            LOG_DEBUG("(Now) Payload size: %d", resNow.payload.length());
            JsonDocument doc;
            if (!deserializeJson(doc, resNow.payload)) {
                Settings::instance().weatherTempDay = doc["main"]["temp"] | 0.0f;
                Settings::instance().weatherHumidity = doc["main"]["humidity"] | 0.0f;
                Settings::instance().weatherIcon = doc["weather"][0]["icon"].as<String>();
            } else {
                LOG_ERROR("JSON parse error (current)");
            }
        } else {
            LOG_ERROR("(Now) Error %d; payload: %s", resNow.statusCode, resNow.payload.c_str());
        }

        if (!skipForecast) {
            HTTPResult resFc = httpForecastTask.result();
            if (resFc.success) {
                LOG_DEBUG("(FC) Payload size: %d", resFc.payload.length())
                JsonDocument doc;
                if (!deserializeJson(doc, resFc.payload)) {
                    float minTemp, maxTemp;
                    if (findMinMaxForecast(doc, todayDate, minTemp, maxTemp) ||
                        findMinMaxForecast(doc, tomorrowDate, minTemp, maxTemp)) {
                        // If we are late in the day (>23:00) and there are no more forecasts for today.
                        // Then we accept the min/max for tomorrow
                        Settings::instance().weatherTempMin = minTemp;
                        Settings::instance().weatherTempMax = maxTemp;
                    }
                    else {
                        LOG_WARN("No forecasts found for today or tomorrow");
                    }
                } else {
                    LOG_ERROR("JSON parse error (forecast)");
                }
            }
            else {
                LOG_ERROR("(FC) Error %d; payload: %s", resFc.statusCode, resFc.payload.c_str());
            }
        }
        else {
            LOG_INFO("Forecast fetch skipped, already done for today");
        }

        Settings::instance().save();
        LOG_INFO("Weather update complete");
        EventBus::instance().publish<WEA_ForecastUpdatedEvent>();

        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}
