#include "ImageScreen.h"

ImageScreen::ImageScreen(const String& filename) {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_STATE_DEFAULT);

    lv_obj_t* img = lv_img_create(_screen);
    const String path = "S:" + filename;
    lv_img_set_src(img, path.c_str());
    lv_obj_center(img);
}