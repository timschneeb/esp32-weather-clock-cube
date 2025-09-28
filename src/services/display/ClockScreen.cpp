#include "ClockScreen.h"

#include <time.h>
#include "Settings.h"

const char *daysShort[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {
    "Jan.", "Feb.", "March", "April", "May", "June", "July", "August", "Sept.", "Oct.", "Nov.", "Dec."
};

void ClockScreen::draw(lv_obj_t* screen) {
    _screen = screen;
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_PART_MAIN);

    date_label = lv_label_create(_screen);
    lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 5);

    time_label = lv_label_create(_screen);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);

    temp_label = lv_label_create(_screen);
    lv_obj_align(temp_label, LV_ALIGN_LEFT_MID, 10, 20);

    humidity_label = lv_label_create(_screen);
    lv_obj_align(humidity_label, LV_ALIGN_LEFT_MID, 120, 20);

    temp_min_label = lv_label_create(_screen);
    lv_obj_align(temp_min_label, LV_ALIGN_BOTTOM_LEFT, 10, -40);

    temp_max_label = lv_label_create(_screen);
    lv_obj_align(temp_max_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    weather_icon = lv_img_create(_screen);
    lv_obj_align(weather_icon, LV_ALIGN_RIGHT_MID, -10, 20);

    update();
}

void ClockScreen::update() {
    time_t now = time(nullptr);
    tm *tm_info = localtime(&now);
    if (!tm_info) return;

    // Date
    String enDate = String(daysShort[tm_info->tm_wday]) + " " +
                    String(tm_info->tm_mday) + " " +
                    String(months[tm_info->tm_mon]);
    lv_label_set_text(date_label, enDate.c_str());

    // Time
    char timeBuffer[9];
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", tm_info);
    lv_label_set_text(time_label, timeBuffer);

    // Temperature
    auto tempValue = String(Settings::instance().weatherTempDay.load(), 1);
    lv_label_set_text_fmt(temp_label, "%s C", tempValue.c_str());

    auto humidityValue = String(static_cast<int>(Settings::instance().weatherHumidity));
    lv_label_set_text_fmt(humidity_label, "%s %%", humidityValue.c_str());

    auto tempMinValue = String(Settings::instance().weatherTempMin, 1);
    lv_label_set_text_fmt(temp_min_label, "Min: %s C", tempMinValue.c_str());

    auto tempMaxValue = String(Settings::instance().weatherTempMax, 1);
    lv_label_set_text_fmt(temp_max_label, "Max: %s C", tempMaxValue.c_str());

    String icon = Settings::instance().weatherIcon.load();
    if (icon != this->lastDrawnWeatherIcon) {
        showWeatherIcon(icon);
        this->lastDrawnWeatherIcon = icon;
    }
}

void ClockScreen::showWeatherIcon(const String& iconCode) {
    String path = "S:/icons/" + iconCode + ".jpg";
    lv_img_set_src(weather_icon, path.c_str());
}