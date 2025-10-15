//
// Created by tim on 15.10.25.
//

#ifndef ESP32_WEATHER_CLOCK_CUBE_QEMUFRAMEBUFFER_H
#define ESP32_WEATHER_CLOCK_CUBE_QEMUFRAMEBUFFER_H

#include <cstdint>

#include <esp_lcd_types.h>

#include "IDisplay.h"

class QEMUFramebuffer final : public virtual IDisplay {
public:
    QEMUFramebuffer();
    ~QEMUFramebuffer() override;
    void push(uint16_t *buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
    void panic(const char *msg, const char *footer) override;

private:
    esp_lcd_panel_handle_t panel_handle = nullptr;
};


#endif //ESP32_WEATHER_CLOCK_CUBE_QEMUFRAMEBUFFER_H