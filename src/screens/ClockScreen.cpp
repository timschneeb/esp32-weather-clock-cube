#include "ClockScreen.h"

#include <time.h>
#include "Settings.h"

const char *daysShort[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {
    "Jan.", "Feb.", "March", "April", "May", "June", "July", "August", "Sept.", "Oct.", "Nov.", "Dec."
};

lv_obj_t * row_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t main;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&main);
        lv_style_set_bg_opa(&main, 0);
        lv_style_set_border_width(&main, 0);
        lv_style_set_pad_all(&main, 0);
        lv_style_set_width(&main, LV_SIZE_CONTENT);
        lv_style_set_height(&main, LV_SIZE_CONTENT);
        lv_style_set_layout(&main, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&main, LV_FLEX_FLOW_ROW);
        lv_style_set_radius(&main, 0);

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);
    lv_obj_add_style(lv_obj_0, &main, 0);


    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}

lv_obj_t * column_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t main;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&main);
        lv_style_set_bg_opa(&main, 0);
        lv_style_set_border_width(&main, 0);
        lv_style_set_pad_row(&main, 5);
        lv_style_set_width(&main, LV_SIZE_CONTENT);
        lv_style_set_height(&main, LV_SIZE_CONTENT);
        lv_style_set_layout(&main, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&main, LV_FLEX_FLOW_COLUMN);
        lv_style_set_radius(&main, 0);

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);
    lv_obj_add_style(lv_obj_0, &main, 0);


    LV_TRACE_OBJ_CREATE("finished");

    return lv_obj_0;
}


void ClockScreen::draw(lv_obj_t *screen) {
    _screen = screen;
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_STATE_DEFAULT);

    // Styles
    static lv_style_t main;
    static lv_style_t label_current_weather;
    static lv_style_t label_white_center;
    static lv_style_t icon_placeholder;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&main);
        lv_style_set_pad_all(&main, 0);
        lv_style_set_border_width(&main, 0);
        lv_style_set_layout(&main, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&main, LV_FLEX_FLOW_COLUMN);
        lv_style_set_pad_row(&main, 0);

        lv_style_init(&label_current_weather);
        lv_style_set_text_color(&label_current_weather, lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&label_current_weather, &lv_font_montserrat_32);
        lv_style_set_text_align(&label_current_weather, LV_TEXT_ALIGN_CENTER);

        lv_style_init(&label_white_center);
        lv_style_set_text_align(&label_white_center, LV_TEXT_ALIGN_CENTER);
        lv_style_set_text_color(&label_white_center, lv_color_hex(0xFFFFFF));

        lv_style_init(&icon_placeholder);
        lv_style_set_bg_color(&icon_placeholder, lv_color_hex(0x222222));
        lv_style_set_border_color(&icon_placeholder, lv_color_hex(0xFFFFFF));
        lv_style_set_border_width(&icon_placeholder, 2);
        lv_style_set_radius(&icon_placeholder, 8);

        style_inited = true;
    }

    // ----------------------------------------

    lv_obj_t *lv_obj_0 = lv_obj_create(_screen);
    lv_obj_set_width(lv_obj_0, 240);
    lv_obj_set_height(lv_obj_0, 240);
    lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x000000), 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_add_style(lv_obj_0, &main, 0);

    lv_obj_t *column_0 = column_create(lv_obj_0);
    lv_obj_set_width(column_0, 240);
    lv_obj_set_height(column_0, 240);
    lv_obj_set_flex_flow(column_0, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flag(column_0, LV_OBJ_FLAG_SCROLLABLE, false);

    date_label = lv_label_create(column_0);
    lv_obj_set_width(date_label, lv_pct(100));
    lv_label_set_text(date_label, "Mon 1 Jan.");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_add_style(date_label, &label_white_center, 0);


    time_label = lv_label_create(column_0);
    lv_obj_set_width(time_label, lv_pct(100));
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_set_height(time_label, 55);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_add_style(time_label, &label_white_center, 0);


    lv_obj_t *row_0 = row_create(column_0);
    lv_obj_set_width(row_0, lv_pct(100));
    lv_obj_set_height(row_0, 32);
    lv_obj_set_flex_flow(row_0, LV_FLEX_FLOW_ROW);
    lv_obj_set_flag(row_0, LV_OBJ_FLAG_SCROLLABLE, false);

    temp_label = lv_label_create(row_0);
    lv_label_set_text(temp_label, "21.5°C");
    lv_obj_set_width(temp_label, lv_pct(49));
    lv_obj_add_style(temp_label, &label_current_weather, 0);
    
    humidity_label = lv_label_create(row_0);
    lv_label_set_text(humidity_label, "55%");
    lv_obj_set_width(humidity_label, lv_pct(49));
    lv_obj_add_style(humidity_label, &label_current_weather, 0);
    
    lv_obj_t *row_1 = row_create(column_0);
    lv_obj_set_width(row_1, lv_pct(100));
    lv_obj_set_height(row_1, 80);
    lv_obj_set_flex_flow(row_1, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(row_1, 0, 0);
    lv_obj_set_style_pad_column(row_1, 0, 0);
    lv_obj_set_flag(row_1, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_t *column_1 = column_create(row_1);
    lv_obj_set_width(column_1, 132);
    lv_obj_set_height(column_1, 60);
    lv_obj_set_style_margin_top(column_1, 11, 0);
    lv_obj_set_flex_flow(column_1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(column_1, 8, 0);
    lv_obj_set_flag(column_1, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_set_style_pad_all(column_1, 0, 0);

    temp_min_label = lv_label_create(column_1);
    lv_label_set_text(temp_min_label, "Min: 18.0°C");
    lv_obj_set_height(temp_min_label, 24);
    lv_obj_set_style_text_font(temp_min_label, &lv_font_montserrat_24, 0);
    lv_obj_set_flex_grow(temp_min_label, 1);
    lv_obj_set_style_text_color(temp_min_label, lv_color_hex(0xFFFFFF), 0);


    temp_max_label = lv_label_create(column_1);
    lv_label_set_text(temp_max_label, "Max: 29.8°C");
    lv_obj_set_height(temp_max_label, 24);
    lv_obj_set_align(temp_max_label, LV_ALIGN_CENTER);
    lv_obj_set_flex_grow(temp_max_label, 1);
    lv_obj_set_style_text_font(temp_max_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(temp_max_label, lv_color_hex(0xFFFFFF), 0);


    weather_icon = lv_image_create(row_1);
    lv_obj_set_size(weather_icon, 90, 90);
    lv_obj_set_align(weather_icon, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_style_margin_top(weather_icon, -3, 0);

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