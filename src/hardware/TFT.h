//
// Created by tim on 29.09.25.
//

#ifndef ESP32_CUBE_TFT_H
#define ESP32_CUBE_TFT_H

#include <TFT_eSPI.h>

#include "IDisplay.h"

class TFT final : public virtual IDisplay {
public:
    TFT();
    void push(uint16_t *buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
    void panic(const char *msg, const char *footer) override;

private:
    TFT_eSPI tft;
};


#endif //ESP32_CUBE_TFT_H