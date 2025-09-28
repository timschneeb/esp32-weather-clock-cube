#ifndef ERRORSCREEN_H
#define ERRORSCREEN_H

#include "services/display/Screen.h"
#include <Arduino.h>

class ErrorScreen final : public Screen {
public:
    ErrorScreen(const String& message);
    void draw(lv_obj_t* screen) override;

private:
    String message;
};

#endif //ERRORSCREEN_H