//
// Created by tim on 25.09.25.
//

#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <ArduinoJson.h>

#include "Task.h"
#include "utils/HTTPRequest.h"
#include "utils/Macros.h"

class WeatherService final : public Task<4096, Priority::Background> {
    TASK_NO_CTOR(WeatherService)
protected:
    [[noreturn]] void run() override;

private:
    HTTPRequest httpNowTask;
    HTTPRequest httpForecastTask;

    static String currentLocalDate(int dayOffset);
    static bool findMinMaxForecast(JsonDocument doc, const String &date, float &minTemp, float &maxTemp);
};



#endif //WEATHERSERVICE_H
