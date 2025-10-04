#ifndef ERRORSCREEN_H
#define ERRORSCREEN_H

#include "Screen.h"
#include <Arduino.h>

class ErrorScreen final : public Screen {
public:
    explicit ErrorScreen(const String& message);
};

#endif //ERRORSCREEN_H