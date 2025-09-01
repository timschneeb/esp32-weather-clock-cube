#ifndef BUTTON_H
#define BUTTON_H

#define BUTTON2_HAS_STD_FUNCTION

#include <Button2.h>

#ifndef TOUCH_PIN
#define TOUCH_PIN 9
#endif

extern "C" {
    typedef void (*OnClickEventHandler)();
}


class Button {
    Button2 button2;
    int threshold = 1500; // ESP32S3
    byte buttonState = HIGH;
    OnClickEventHandler onClick = nullptr;
    OnClickEventHandler onLongPress = nullptr;

public:
    Button();
    void begin();
    void tick();

    // save function for click event
    void attachClick(OnClickEventHandler newFunction);

    void attachLongPressStart(OnClickEventHandler newFunction);

    bool touchdetected = false;
};


#endif // BUTTON_H
