/**
 * @file header_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "header_gen.h"
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

lv_obj_t * header_create(lv_obj_t * parent, const char * title)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t main0;
    static lv_style_t edited;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&main0);
        lv_style_set_bg_color(&main0, lv_color_hex(0x2d2d2d));
        lv_style_set_border_width(&main0, 2);
        lv_style_set_border_side(&main0, LV_BORDER_SIDE_BOTTOM);
        lv_style_set_border_color(&main0, lv_color_hex(0xa2a2a2));
        lv_style_set_radius(&main0, 0);
        lv_style_set_width(&main0, lv_pct(100));
        lv_style_set_height(&main0, 32);
        lv_style_set_pad_hor(&main0, 8);
        lv_style_set_layout(&main0, LV_LAYOUT_FLEX);
        lv_style_set_flex_cross_place(&main0, LV_FLEX_ALIGN_CENTER);
        lv_style_set_flex_track_place(&main0, LV_FLEX_ALIGN_CENTER);
        lv_style_set_flex_flow(&main0, LV_FLEX_FLOW_ROW);
        lv_style_set_text_color(&main0, lv_color_hex(0xffffff));

        lv_style_init(&edited);
        lv_style_set_text_color(&edited, lv_color_hex(0x0099ee));

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);

    lv_obj_add_style(lv_obj_0, &main0, 0);
    lv_obj_t * subtitle_0 = subtitle_create(lv_obj_0, title);
    lv_obj_set_flex_grow(subtitle_0, 1);
    
    lv_obj_t * row_0 = row_create(lv_obj_0);
    lv_obj_set_width(row_0, 40);
    lv_obj_set_style_flex_main_place(row_0, LV_FLEX_ALIGN_END, 0);
    lv_obj_set_style_text_color(row_0, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_column(row_0, 0, 0);
    lv_obj_t * subtitle_1 = subtitle_create(row_0, "settings");
    lv_label_bind_text(subtitle_1, &hours, NULL);
    
    lv_obj_t * subtitle_2 = subtitle_create(row_0, ":");
    
    lv_obj_t * subtitle_3 = subtitle_create(row_0, "settings");
    lv_label_bind_text(subtitle_3, &mins, NULL);

    LV_TRACE_OBJ_CREATE("finished");

    lv_obj_set_name(lv_obj_0, "header_#");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

