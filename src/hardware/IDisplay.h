//
// Created by tim on 15.10.25.
//

#ifndef ESP32_WEATHER_CLOCK_CUBE_IDISPLAY_H
#define ESP32_WEATHER_CLOCK_CUBE_IDISPLAY_H

#include <cstdint>

class IDisplay {
public:
    virtual ~IDisplay() = default;
    virtual void push(uint16_t *buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;
    virtual void panic(const char *msg, const char *footer) = 0;
};

#endif //ESP32_WEATHER_CLOCK_CUBE_IDISPLAY_H