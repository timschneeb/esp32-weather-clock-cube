//
// Created by tim on 05.10.25.
//

#ifndef ESP32_WEATHER_CLOCK_CUBE_DEBUGSCREEN_H
#define ESP32_WEATHER_CLOCK_CUBE_DEBUGSCREEN_H
#include "Screen.h"


class DebugScreen final : public Screen {
public:
    DebugScreen();
protected:
    void update() override;
};


#endif //ESP32_WEATHER_CLOCK_CUBE_DEBUGSCREEN_H