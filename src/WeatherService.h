//
// Created by tim on 25.09.25.
//

#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include "Task.h"
#include "utils/HTTPRequest.h"
#include "utils/Macros.h"

class WeatherService final : public Task {
    SINGLETON(WeatherService)

protected:
    [[noreturn]] void run() override;

private:
    HTTPRequest httpNowTask;
    HTTPRequest httpForecastTask;
};



#endif //WEATHERSERVICE_H
