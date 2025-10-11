#ifndef CLOCKSCREEN_H
#define CLOCKSCREEN_H

#include "Screen.h"
#include <WString.h>

class ClockScreen final : public Screen {
public:
    ClockScreen();
    void update() override;

private:
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