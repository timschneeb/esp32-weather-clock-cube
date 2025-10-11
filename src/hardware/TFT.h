//
// Created by tim on 29.09.25.
//

#ifndef GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_TFT_H
#define GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_TFT_H

#include <TFT_eSPI.h>

class TFT {
public:
    TFT();
    void push(uint16_t *buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void panic(const char *msg, const char *footer);

private:
    TFT_eSPI tft;
};


#endif //GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_TFT_H