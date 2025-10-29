//
// Created by tim on 29.10.25.
//

#ifndef ESP32_WEATHER_CLOCK_CUBE_POWER_H
#define ESP32_WEATHER_CLOCK_CUBE_POWER_H

#include <functional>

class Power {
public:
    void setOnSleepStateChanged(const std::function<void(bool)> &callback);

    void sleep() const;
    void wake() const;
    void toggleManualSleep() const;

    static bool isSleeping();
    static bool isSleepingByPowerButton();

private:
    std::function<void(bool)> onSleepStateChanged;
};


#endif //ESP32_WEATHER_CLOCK_CUBE_POWER_H