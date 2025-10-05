/**
 * @file gui_gen.h
 */

#ifndef GUI_GEN_H
#define GUI_GEN_H

#ifndef UI_SUBJECT_STRING_LENGTH
#define UI_SUBJECT_STRING_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/*-------------------
 * Permanent screens
 *------------------*/

/*----------------
 * Global styles
 *----------------*/

extern lv_style_t label_white_center;

/*----------------
 * Fonts
 *----------------*/

extern lv_font_t * montserrat_14_c_array;

extern lv_font_t * montserrat_22_c_array;

extern lv_font_t * montserrat_28_c_array;

extern lv_font_t * montserrat_22_c_array;

extern lv_font_t * montserrat_24_c_array;

extern lv_font_t * montserrat_32_c_array;

extern lv_font_t * montserrat_30_c_array;

extern lv_font_t * montserrat_48_c_array;

/*----------------
 * Images
 *----------------*/

extern const void * img_wifi;
extern const void * img_bluetooth;
extern const void * img_bell;
extern const void * img_weather_placeholder;

/*----------------
 * Subjects
 *----------------*/

extern lv_subject_t hours;
extern lv_subject_t mins;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*----------------
 * Event Callbacks
 *----------------*/

/**
 * Initialize the component library
 */

void gui_init_gen(const char * asset_path);

/**********************
 *      MACROS
 **********************/

/**********************
 *   POST INCLUDES
 **********************/

/*Include all the widget and components of this library*/
#include "components/column/column_gen.h"
#include "components/header/header_gen.h"
#include "components/row/row_gen.h"
#include "components/subtitle/subtitle_gen.h"
#include "components/title/title_gen.h"
#include "screens/clock_screen/clock_screen_gen.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*GUI_GEN_H*/