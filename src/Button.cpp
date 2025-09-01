#include "Button.h"

Button::Button() {
    onClick = [] {};
    onLongPress = [] {};
}

void Button::tick() {
    button2.loop();
    if (touchdetected) {
        touchdetected = false;
        if (touchInterruptGetLastStatus(TOUCH_PIN)) {
            buttonState = LOW;
        } else {
            buttonState = HIGH;
        }
    }
}

// save function for click event
void Button::attachClick(OnClickEventHandler newFunction) {
    onClick = newFunction;
}

void Button::attachLongPressStart(OnClickEventHandler newFunction) {
    onLongPress = newFunction;
}

void gotTouch(void *self) {
    static_cast<Button *>(self)->touchdetected = true;
}

void Button::begin() {
    touchAttachInterruptArg(TOUCH_PIN, gotTouch, this, threshold);
    button2.setButtonStateFunction([this] { return buttonState; });
    button2.setClickHandler([this](Button2 &) { if (onClick != nullptr) onClick(); });
    button2.setLongClickTime(500);
    button2.setLongClickDetectedHandler([this](Button2 &) { if (onLongPress != nullptr) onLongPress(); });
    button2.begin(BTN_VIRTUAL_PIN);
}
