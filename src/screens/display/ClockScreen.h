#ifndef CLOCKSCREEN_H
#define CLOCKSCREEN_H

#include "Screen.h"
#include <Arduino.h>

class ClockScreen final : public Screen {
public:
    void draw(lv_obj_t* screen) override;
    void update() override;

private:
    void showWeatherIcon(const String& iconCode);

    lv_obj_t *time_label = nullptr;
    lv_obj_t *date_label = nullptr;
    lv_obj_t *temp_label = nullptr;
    lv_obj_t *humidity_label = nullptr;
    lv_obj_t *temp_min_label = nullptr;
    lv_obj_t *temp_max_label = nullptr;
    lv_obj_t *weather_icon = nullptr;

    String lastDrawnWeatherIcon = "";
};

#endif //CLOCKSCREEN_H