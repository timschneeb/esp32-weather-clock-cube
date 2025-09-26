//
// Created by tim on 25.09.25.
//

#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H



#include <Arduino.h>
#include "HTTPRequest.h"
#include "utils/Macros.h"

class WeatherService final {
    SINGLETON(WeatherService)
public:
    void run(void *pvParameters);

private:
    HTTPRequest httpNowTask;
    HTTPRequest httpForecastTask;
};



#endif //WEATHERSERVICE_H
