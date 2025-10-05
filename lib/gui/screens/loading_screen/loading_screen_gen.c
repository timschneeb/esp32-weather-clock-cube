/**
 * @file loading_screen_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "loading_screen_gen.h"
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

lv_obj_t * loading_screen_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t root;
    static lv_style_t label_current_weather;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&root);
        lv_style_set_pad_all(&root, 0);
        lv_style_set_border_width(&root, 0);
        lv_style_set_layout(&root, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&root, LV_FLEX_FLOW_COLUMN);
        lv_style_set_pad_row(&root, 0);

        lv_style_init(&label_current_weather);
        lv_style_set_text_color(&label_current_weather, lv_color_hex(0xFFFFFF));
        lv_style_set_text_font(&label_current_weather, montserrat_32_c_array);
        lv_style_set_text_align(&label_current_weather, LV_TEXT_ALIGN_CENTER);

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
    lv_obj_set_width(lv_obj_0, lv_pct(100));
    lv_obj_set_height(lv_obj_0, lv_pct(100));
    lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x000000), 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_add_style(lv_obj_0, &root, 0);
    lv_obj_t * column_0 = column_create(lv_obj_0);
    lv_obj_set_width(column_0, lv_pct(100));
    lv_obj_set_height(column_0, lv_pct(100));
    lv_obj_set_style_pad_row(column_0, 12, 0);
    lv_obj_set_flex_flow(column_0, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(column_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(column_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_flag(column_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_t * lv_arc_0 = lv_arc_create(column_0);
    lv_obj_set_height(lv_arc_0, 50);
    lv_obj_set_width(lv_arc_0, 50);
    
    lv_obj_t * text = lv_label_create(column_0);
    lv_obj_set_name(text, "text");
    lv_obj_set_width(text, lv_pct(100));
    lv_label_set_text(text, "Loading...");
    lv_obj_set_style_text_font(text, montserrat_22_c_array, 0);
    lv_obj_add_style(text, &label_white_center, 0);

    LV_TRACE_OBJ_CREATE("finished");

    lv_obj_set_name(lv_obj_0, "loading_screen");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

