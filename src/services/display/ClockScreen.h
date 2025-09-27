#ifndef CLOCKSCREEN_H
#define CLOCKSCREEN_H

#include "Screen.h"

class ClockScreen final : public Screen {
public:
    void draw(TFT_eSPI& tft) override;
    void update(TFT_eSPI& tft) override;

private:
    void showWeatherIconJPG(TFT_eSPI& tft, const String& iconCode);
    String lastDrawnWeatherIcon = "";
    String lastDate = "";
    unsigned long lastClockUpdate = 0;
};

#endif //CLOCKSCREEN_H
