#ifndef SCREEN_H
#define SCREEN_H

#include <TFT_eSPI.h>

class Screen {
public:
    virtual ~Screen() = default;

    virtual void draw(TFT_eSPI& tft) = 0;
    virtual void update(TFT_eSPI& tft) = 0;
};

#endif //SCREEN_H
