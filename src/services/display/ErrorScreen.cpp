#include "ErrorScreen.h"

ErrorScreen::ErrorScreen(const String& message) : message(message) {}

void ErrorScreen::draw(lv_obj_t* screen) {
    _screen = screen;
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_STATE_DEFAULT);

    lv_obj_t* label = lv_label_create(_screen);
    lv_label_set_text(label, "Error:");
    lv_obj_set_style_text_color(label, lv_color_hex(0xff0000), LV_STATE_DEFAULT);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t* msg_label = lv_label_create(_screen);
    lv_label_set_text(msg_label, message.c_str());
    lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, 0);
}