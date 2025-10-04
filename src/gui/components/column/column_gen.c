/**
 * @file column_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "column_gen.h"
#include "gui.h"

/*********************
 *      DEFINES
 *********************/

#define SPACE_MD 5

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

lv_obj_t * column_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t main0;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&main0);
        lv_style_set_bg_opa(&main0, 0);
        lv_style_set_border_width(&main0, 0);
        lv_style_set_pad_row(&main0, SPACE_MD);
        lv_style_set_width(&main0, LV_SIZE_CONTENT);
        lv_style_set_height(&main0, LV_SIZE_CONTENT);
        lv_style_set_layout(&main0, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&main0, LV_FLEX_FLOW_COLUMN);
        lv_style_set_radius(&main0, 0);

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);

    lv_obj_add_style(lv_obj_0, &main0, 0);

    LV_TRACE_OBJ_CREATE("finished");

    lv_obj_set_name(lv_obj_0, "column_#");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

