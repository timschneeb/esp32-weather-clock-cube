/**
 * @file clock_screen_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "clock_screen_gen.h"
#include "gui.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * clock_screen_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t main0;
    static lv_style_t label_current_weather;
    static lv_style_t label_white_center;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&main0);
        lv_style_set_pad_all(&main0, 0);
        lv_style_set_border_width(&main0, 0);
        lv_style_set_layout(&main0, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&main0, LV_FLEX_FLOW_COLUMN);
        lv_style_set_pad_row(&main0, 0);

        lv_style_init(&label_current_weather);
        lv_style_set_text_color(&label_current_weather, lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&label_current_weather, montserrat_32_c_array);
        lv_style_set_text_align(&label_current_weather, LV_TEXT_ALIGN_CENTER);

        lv_style_init(&label_white_center);
        lv_style_set_text_align(&label_white_center, LV_TEXT_ALIGN_CENTER);
        lv_style_set_text_color(&label_white_center, lv_color_hex(0xFFFFFF));

        style_inited = true;
    }

    if (clock_screen == NULL) clock_screen = lv_obj_create(NULL);
    lv_obj_t * lv_obj_0 = clock_screen;
    lv_obj_set_width(lv_obj_0, 240);
    lv_obj_set_height(lv_obj_0, 240);
    lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x000000), 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_add_style(lv_obj_0, &main0, 0);
    lv_obj_t * column_0 = column_create(lv_obj_0);
    lv_obj_set_width(column_0, 240);
    lv_obj_set_height(column_0, 240);
    lv_obj_set_flex_flow(column_0, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flag(column_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_t * date = lv_label_create(column_0);
    lv_obj_set_name(date, "date");
    lv_obj_set_width(date, lv_pct(100));
    lv_obj_set_height(date, 28);
    lv_label_set_text(date, "Mon 1 Jan.");
    lv_obj_set_style_text_font(date, montserrat_20_c_array, 0);
    lv_obj_add_style(date, &label_white_center, 0);
    
    lv_obj_t * time = lv_label_create(column_0);
    lv_obj_set_name(time, "time");
    lv_obj_set_width(time, lv_pct(100));
    lv_obj_set_height(time, 55);
    lv_label_set_text(time, "00:00:00");
    lv_obj_set_style_text_font(time, montserrat_48_c_array, 0);
    lv_obj_add_style(time, &label_white_center, 0);
    
    lv_obj_t * row_0 = row_create(column_0);
    lv_obj_set_width(row_0, lv_pct(100));
    lv_obj_set_height(row_0, 32);
    lv_obj_set_flex_flow(row_0, LV_FLEX_FLOW_ROW);
    lv_obj_set_flag(row_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_t * temperature = lv_label_create(row_0);
    lv_obj_set_name(temperature, "temperature");
    lv_label_set_text(temperature, "21.5°C");
    lv_obj_set_width(temperature, lv_pct(49));
    lv_obj_add_style(temperature, &label_current_weather, 0);
    
    lv_obj_t * humidity = lv_label_create(row_0);
    lv_obj_set_name(humidity, "humidity");
    lv_label_set_text(humidity, "55%");
    lv_obj_set_width(humidity, lv_pct(49));
    lv_obj_add_style(humidity, &label_current_weather, 0);
    
    lv_obj_t * row_1 = row_create(column_0);
    lv_obj_set_width(row_1, lv_pct(100));
    lv_obj_set_height(row_1, 80);
    lv_obj_set_flex_flow(row_1, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(row_1, 0, 0);
    lv_obj_set_style_pad_column(row_1, 0, 0);
    lv_obj_set_flag(row_1, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_t * column_1 = column_create(row_1);
    lv_obj_set_width(column_1, 132);
    lv_obj_set_height(column_1, 60);
    lv_obj_set_style_margin_top(column_1, 11, 0);
    lv_obj_set_flex_flow(column_1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(column_1, 8, 0);
    lv_obj_set_flag(column_1, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_set_style_pad_all(column_1, 0, 0);
    lv_obj_t * min_temperature = lv_label_create(column_1);
    lv_obj_set_name(min_temperature, "min_temperature");
    lv_label_set_text(min_temperature, "Min: 8.0°C");
    lv_obj_set_height(min_temperature, 0);
    lv_obj_set_style_text_font(min_temperature, montserrat_20_c_array, 0);
    lv_obj_set_flex_grow(min_temperature, 1);
    lv_obj_set_style_text_color(min_temperature, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t * max_temperature = lv_label_create(column_1);
    lv_obj_set_name(max_temperature, "max_temperature");
    lv_label_set_text(max_temperature, "Max: 88.8°C");
    lv_obj_set_height(max_temperature, 24);
    lv_obj_set_align(max_temperature, LV_ALIGN_CENTER);
    lv_obj_set_flex_grow(max_temperature, 1);
    lv_obj_set_style_text_font(max_temperature, montserrat_20_c_array, 0);
    lv_obj_set_style_text_color(max_temperature, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t * weather_icon = lv_image_create(row_1);
    lv_obj_set_name(weather_icon, "weather_icon");
    lv_obj_set_width(weather_icon, 90);
    lv_obj_set_height(weather_icon, 90);
    lv_obj_set_align(weather_icon, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_style_margin_top(weather_icon, -3, 0);
    lv_image_set_src(weather_icon, img_weather_02d);

    LV_TRACE_OBJ_CREATE("finished");

    lv_obj_set_name(lv_obj_0, "clock_screen");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

