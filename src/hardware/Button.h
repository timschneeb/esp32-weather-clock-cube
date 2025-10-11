#ifndef BUTTON_H
#define BUTTON_H

class Button2;

#include <functional>

#include "Config.h"

typedef std::function<void()> OnClickEventHandler;

class Button {
public:
    Button();
    ~Button();

    void begin();
    void tick();

    // save function for click event
    void attachClick(const OnClickEventHandler &newFunction);
    void attachLongPressStart(const OnClickEventHandler &newFunction);

    bool touchDetected = false;

private:
    Button2* button2;
    int threshold = TOUCH_THRESHOLD; // ESP32S3
    uint8_t buttonState = 1;
    OnClickEventHandler onClick = nullptr;
    OnClickEventHandler onLongPress = nullptr;
};


#endif // BUTTON_H
