//
// Created by tim on 06.10.25.
//

#ifndef ESP32_WEATHER_CLOCK_CUBE_LOADINGSCREEN_H
#define ESP32_WEATHER_CLOCK_CUBE_LOADINGSCREEN_H

#include <Arduino.h>

#include "Screen.h"

class LoadingScreen final : public Screen {
public:
    LoadingScreen();
    explicit LoadingScreen(const String& message);
private:
    lv_obj_t* text = nullptr;
};


#endif //ESP32_WEATHER_CLOCK_CUBE_LOADINGSCREEN_H