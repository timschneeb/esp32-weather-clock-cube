#ifndef STATUSSCREEN_H
#define STATUSSCREEN_H

#include "Screen.h"
#include <Arduino.h>

class StatusScreen final : public Screen {
public:
    StatusScreen(const String& message, unsigned long duration);
    void draw(lv_obj_t* screen) override;
    bool isExpired() const;

private:
    String message;
    unsigned long startTime;
    unsigned long timeout;
};

#endif //STATUSSCREEN_H