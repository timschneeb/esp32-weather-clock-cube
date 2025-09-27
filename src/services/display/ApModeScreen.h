#ifndef APMODESCREEN_H
#define APMODESCREEN_H

#include "Screen.h"

class ApModeScreen final : public Screen {
public:
    void draw(TFT_eSPI& tft) override;
    void update(TFT_eSPI& tft) override;
};

#endif //APMODESCREEN_H
