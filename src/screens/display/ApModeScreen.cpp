#include "ApModeScreen.h"
#include <WiFi.h>
#include "Config.h"

void ApModeScreen::draw(lv_obj_t* screen) {
    _screen = screen;
    lv_obj_set_style_bg_color(_screen, lv_color_hex(0x000000), LV_STATE_DEFAULT);

    lv_obj_t* title_label = lv_label_create(_screen);
    lv_label_set_text(title_label, "AP MODE");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, LV_STATE_DEFAULT);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t* ssid_label = lv_label_create(_screen);
    lv_label_set_text_fmt(ssid_label, "SSID: %s", DEFAULT_SSID);
    lv_obj_align(ssid_label, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t* pwd_label = lv_label_create(_screen);
    lv_label_set_text_fmt(pwd_label, "Password: %s", DEFAULT_PASSWORD);
    lv_obj_align(pwd_label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* ip_label = lv_label_create(_screen);
    lv_label_set_text_fmt(ip_label, "IP: %s", WiFi.softAPIP().toString().c_str());
    lv_obj_align(ip_label, LV_ALIGN_CENTER, 0, 20);
}