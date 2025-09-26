//
// Created by tim on 25.09.25.
//

#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <QuarkTS.h>

#include "HTTPRequest.h"
#include "utils/Macros.h"

class WeatherService final : public qOS::task {
    SINGLETON(WeatherService)
public:
    static void registerTask();

protected:
    void activities(qOS::event_t e) override;

private:
    HTTPRequest httpNowTask;
    HTTPRequest httpForecastTask;
};



#endif //WEATHERSERVICE_H
