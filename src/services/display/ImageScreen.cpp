#include "ImageScreen.h"

ImageScreen::ImageScreen(const String& filename) : filename(filename) {}

void ImageScreen::draw(lv_obj_t* screen) {
    _screen = screen;
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_PART_MAIN);

    lv_obj_t* img = lv_img_create(_screen);
    String path = "S:" + filename;
    lv_img_set_src(img, path.c_str());
    lv_obj_center(img);
}