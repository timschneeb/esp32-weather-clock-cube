#include "StatusScreen.h"

StatusScreen::StatusScreen(const String& message, unsigned long duration) : message(message), startTime(millis()), timeout(duration) {}

void StatusScreen::draw(lv_obj_t* screen) {
    _screen = screen;
    // This is an overlay, so we don't clear the screen
    lv_obj_t* label = lv_label_create(lv_layer_top());
    lv_label_set_text(label, message.c_str());
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

bool StatusScreen::isExpired() const {
    return millis() - startTime > timeout;
}