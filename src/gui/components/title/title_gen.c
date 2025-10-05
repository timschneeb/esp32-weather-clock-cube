/**
 * @file title_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "title_gen.h"
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

lv_obj_t * title_create(lv_obj_t * parent, const char * text)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t root;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&root);
        lv_style_set_text_font(&root, montserrat_22_c_array);

        style_inited = true;
    }

    lv_obj_t * lv_label_0 = lv_label_create(parent);
    lv_label_set_text(lv_label_0, text);

    lv_obj_add_style(lv_label_0, &root, 0);

    LV_TRACE_OBJ_CREATE("finished");

    lv_obj_set_name(lv_label_0, "title_#");

    return lv_label_0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

