/**
 * @file gui_gen.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "gui_gen.h"

#if LV_USE_XML
#endif /* LV_USE_XML */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/*----------------
 * Translations
 *----------------*/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/*--------------------
 *  Permanent screens
 *-------------------*/

/*----------------
 * Global styles
 *----------------*/

lv_style_t label_white_center;

/*----------------
 * Fonts
 *----------------*/

lv_font_t * montserrat_14_c_array;
extern lv_font_t montserrat_14_c_array_data;
lv_font_t * montserrat_22_c_array;
extern lv_font_t montserrat_22_c_array_data;
lv_font_t * montserrat_28_c_array;
extern lv_font_t montserrat_28_c_array_data;
lv_font_t * montserrat_22_c_array;
extern lv_font_t montserrat_22_c_array_data;
lv_font_t * montserrat_24_c_array;
extern lv_font_t montserrat_24_c_array_data;
lv_font_t * montserrat_32_c_array;
extern lv_font_t montserrat_32_c_array_data;
lv_font_t * montserrat_30_c_array;
extern lv_font_t montserrat_30_c_array_data;
lv_font_t * montserrat_48_c_array;
extern lv_font_t montserrat_48_c_array_data;

/*----------------
 * Images
 *----------------*/

const void * img_wifi;
const void * img_bluetooth;
const void * img_bell;
const void * img_weather_placeholder;

/*----------------
 * Subjects
 *----------------*/

lv_subject_t hours;
lv_subject_t mins;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void gui_init_gen(const char * asset_path)
{
    char buf[256];

    /*----------------
     * Global styles
     *----------------*/

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&label_white_center);
        lv_style_set_text_align(&label_white_center, LV_TEXT_ALIGN_CENTER);
        lv_style_set_text_color(&label_white_center, lv_color_hex(0xFFFFFF));

        style_inited = true;
    }

    /*----------------
     * Fonts
     *----------------*/

    /* get font 'montserrat_14_c_array' from a C array */
    montserrat_14_c_array = &montserrat_14_c_array_data;
    /* get font 'montserrat_22_c_array' from a C array */
    montserrat_22_c_array = &montserrat_22_c_array_data;
    /* get font 'montserrat_28_c_array' from a C array */
    montserrat_28_c_array = &montserrat_28_c_array_data;
    /* get font 'montserrat_22_c_array' from a C array */
    montserrat_22_c_array = &montserrat_22_c_array_data;
    /* get font 'montserrat_24_c_array' from a C array */
    montserrat_24_c_array = &montserrat_24_c_array_data;
    /* get font 'montserrat_32_c_array' from a C array */
    montserrat_32_c_array = &montserrat_32_c_array_data;
    /* get font 'montserrat_30_c_array' from a C array */
    montserrat_30_c_array = &montserrat_30_c_array_data;
    /* get font 'montserrat_48_c_array' from a C array */
    montserrat_48_c_array = &montserrat_48_c_array_data;


    /*----------------
     * Images
     *----------------*/
    lv_snprintf(buf, 256, "%s%s", asset_path, "images/wifi-solid.png");
    img_wifi = lv_strdup(buf);
    lv_snprintf(buf, 256, "%s%s", asset_path, "images/bluetooth-brands.png");
    img_bluetooth = lv_strdup(buf);
    lv_snprintf(buf, 256, "%s%s", asset_path, "images/bell-solid.png");
    img_bell = lv_strdup(buf);
    lv_snprintf(buf, 256, "%s%s", asset_path, "images/weather/04d.png");
    img_weather_placeholder = lv_strdup(buf);

    /*----------------
     * Subjects
     *----------------*/
    lv_subject_init_int(&hours, 17);
    lv_subject_init_int(&mins, 45);

    /*----------------
     * Translations
     *----------------*/

#if LV_USE_XML
    /* Register widgets */

    /* Register fonts */
    lv_xml_register_font(NULL, "montserrat_14_c_array", montserrat_14_c_array);
    lv_xml_register_font(NULL, "montserrat_22_c_array", montserrat_22_c_array);
    lv_xml_register_font(NULL, "montserrat_28_c_array", montserrat_28_c_array);
    lv_xml_register_font(NULL, "montserrat_22_c_array", montserrat_22_c_array);
    lv_xml_register_font(NULL, "montserrat_24_c_array", montserrat_24_c_array);
    lv_xml_register_font(NULL, "montserrat_32_c_array", montserrat_32_c_array);
    lv_xml_register_font(NULL, "montserrat_30_c_array", montserrat_30_c_array);
    lv_xml_register_font(NULL, "montserrat_48_c_array", montserrat_48_c_array);

    /* Register subjects */
    lv_xml_register_subject(NULL, "hours", &hours);
    lv_xml_register_subject(NULL, "mins", &mins);

    /* Register callbacks */
#endif

    /* Register all the global assets so that they won't be created again when globals.xml is parsed.
     * While running in the editor skip this step to update the preview when the XML changes */
#if LV_USE_XML && !defined(LV_EDITOR_PREVIEW)
    /* Register images */
    lv_xml_register_image(NULL, "img_wifi", img_wifi);
    lv_xml_register_image(NULL, "img_bluetooth", img_bluetooth);
    lv_xml_register_image(NULL, "img_bell", img_bell);
    lv_xml_register_image(NULL, "img_weather_placeholder", img_weather_placeholder);
#endif

#if LV_USE_XML == 0
    /*--------------------
     *  Permanent screens
     *-------------------*/
    /* If XML is enabled it's assumed that the permanent screens are created
     * manaully from XML using lv_xml_create() */
#endif
}

/* Callbacks */

/**********************
 *   STATIC FUNCTIONS
 **********************/