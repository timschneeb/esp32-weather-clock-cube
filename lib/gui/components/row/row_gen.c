/**
 * @file row_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "row_gen.h"
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

lv_obj_t * row_create(lv_obj_t * parent)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t root;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&root);
        lv_style_set_bg_opa(&root, 0);
        lv_style_set_border_width(&root, 0);
        lv_style_set_pad_all(&root, 0);
        lv_style_set_width(&root, LV_SIZE_CONTENT);
        lv_style_set_height(&root, LV_SIZE_CONTENT);
        lv_style_set_layout(&root, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&root, LV_FLEX_FLOW_ROW);
        lv_style_set_radius(&root, 0);

        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(parent);

    lv_obj_add_style(lv_obj_0, &root, 0);

    LV_TRACE_OBJ_CREATE("finished");

    lv_obj_set_name(lv_obj_0, "row_#");

    return lv_obj_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

