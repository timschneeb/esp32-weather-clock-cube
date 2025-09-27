#ifndef BUTTON_H
#define BUTTON_H

#define BUTTON2_HAS_STD_FUNCTION

#include <Button2.h>

#include "Config.h"

typedef std::function<void()> OnClickEventHandler;

class Button {
public:
    Button();
    void begin();
    void tick();

    // save function for click event
    void attachClick(OnClickEventHandler newFunction);
    void attachLongPressStart(OnClickEventHandler newFunction);

    bool touchDetected = false;

private:
    Button2 button2;
    int threshold = TOUCH_THRESHOLD; // ESP32S3
    byte buttonState = HIGH;
    OnClickEventHandler onClick = nullptr;
    OnClickEventHandler onLongPress = nullptr;
};


#endif // BUTTON_H
