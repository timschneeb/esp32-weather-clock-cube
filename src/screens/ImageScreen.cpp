#include "ImageScreen.h"

#include <SPIFFS.h>

#include "utils/Macros.h"

ImageScreen::ImageScreen(const String &filename, const bool deleteLater) : filename(filename), deleteLater(deleteLater) {
    _screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_STATE_DEFAULT);

    lv_obj_t *img = lv_img_create(_screen);
    const String path = "S:" + filename;
    lv_img_set_src(img, path.c_str());
    lv_obj_center(img);
}

ImageScreen::~ImageScreen() {
    if (deleteLater && SPIFFS.exists(filename)) {
        SPIFFS.remove(filename);
        LOG_DEBUG("Deleted image file %s", filename.c_str());
    }
}
