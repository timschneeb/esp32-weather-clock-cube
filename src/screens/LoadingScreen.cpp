//
// Created by tim on 06.10.25.
//

#include "LoadingScreen.h"

#include <WString.h>
#include <gui.h>

LoadingScreen::LoadingScreen() : LoadingScreen(String("Loading...")) {}

LoadingScreen::LoadingScreen(const String& message) {
    static lv_style_t root;
    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&root);
        lv_style_set_pad_all(&root, 0);
        lv_style_set_border_width(&root, 0);
        lv_style_set_layout(&root, LV_LAYOUT_FLEX);
        lv_style_set_flex_flow(&root, LV_FLEX_FLOW_COLUMN);
        lv_style_set_pad_row(&root, 0);
        style_inited = true;
    }

    lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
    lv_obj_set_width(lv_obj_0, lv_pct(100));
    lv_obj_set_height(lv_obj_0, lv_pct(100));
    lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x000000), 0);
    lv_obj_set_flag(lv_obj_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_add_style(lv_obj_0, &root, 0);
    _screen = lv_obj_0;

    lv_obj_t * column_0 = column_create(lv_obj_0);
    lv_obj_set_width(column_0, lv_pct(100));
    lv_obj_set_height(column_0, lv_pct(100));
    lv_obj_set_style_pad_row(column_0, 18, 0);
    lv_obj_set_flex_flow(column_0, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_main_place(column_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_flex_cross_place(column_0, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_flag(column_0, LV_OBJ_FLAG_SCROLLABLE, false);
    lv_obj_t * lv_spinner = lv_spinner_create(column_0);
    lv_spinner_set_anim_params(lv_spinner, 1500, 200);
    lv_obj_set_height(lv_spinner, 75);
    lv_obj_set_width(lv_spinner, 75);

    text = lv_label_create(column_0);
    lv_obj_set_name(text, "text");
    lv_obj_set_width(text, lv_pct(100));
    lv_label_set_text(text, message.c_str());
    lv_obj_set_style_text_font(text, montserrat_22_c_array, 0);
    lv_obj_add_style(text, &label_white_center, 0);

    LV_TRACE_OBJ_CREATE("finished");

    lv_obj_set_name(lv_obj_0, "loading_screen");
}
