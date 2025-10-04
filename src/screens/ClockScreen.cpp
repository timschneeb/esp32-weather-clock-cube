#include "ClockScreen.h"

#include <time.h>

#include "Settings.h"
#include "gui/gui.h"

const char *daysShort[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {
    "Jan.", "Feb.", "March", "April", "May", "June", "July", "August", "Sept.", "Oct.", "Nov.", "Dec."
};

ClockScreen::ClockScreen() {
    _screen = clock_screen_create();
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_STATE_DEFAULT);

    date_label = lv_obj_find_by_name(_screen, "date");
    time_label = lv_obj_find_by_name(_screen, "time");
    temp_label = lv_obj_find_by_name(_screen, "temperature");
    humidity_label = lv_obj_find_by_name(_screen, "humidity");
    temp_min_label = lv_obj_find_by_name(_screen, "min_temperature");
    temp_max_label = lv_obj_find_by_name(_screen, "max_temperature");
    weather_icon = lv_obj_find_by_name(_screen, "weather_icon");

    update();
}

void ClockScreen::update() {
    const time_t now = time(nullptr);
    const tm *tm_info = localtime(&now);
    if (!tm_info) return;

    // Date
    const String enDate = String(daysShort[tm_info->tm_wday]) + " " +
                    String(tm_info->tm_mday) + " " +
                    String(months[tm_info->tm_mon]);
    lv_label_set_text(date_label, enDate.c_str());

    // Time
    char timeBuffer[9];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", tm_info);
    lv_label_set_text(time_label, timeBuffer);

    // Temperature
    const auto tempValue = String(Settings::instance().weatherTempDay.load(), 1);
    lv_label_set_text_fmt(temp_label, "%s°C", tempValue.c_str());

    const auto humidityValue = String(static_cast<int>(Settings::instance().weatherHumidity));
    lv_label_set_text_fmt(humidity_label, "%s%%", humidityValue.c_str());

    const auto tempMinValue = String(Settings::instance().weatherTempMin, 1);
    lv_label_set_text_fmt(temp_min_label, "Min: %s°C", tempMinValue.c_str());

    const auto tempMaxValue = String(Settings::instance().weatherTempMax, 1);
    lv_label_set_text_fmt(temp_max_label, "Max: %s°C", tempMaxValue.c_str());

    const String icon = Settings::instance().weatherIcon.load();
    if (icon != this->lastDrawnWeatherIcon) {
        const String path = "S:/icons/" + icon + ".jpg";
        lv_img_set_src(weather_icon, path.c_str());
        this->lastDrawnWeatherIcon = icon;
    }
}