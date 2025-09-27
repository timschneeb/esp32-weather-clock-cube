#ifndef ERRORSCREEN_H
#define ERRORSCREEN_H

#include "Screen.h"

class ErrorScreen final : public Screen {
public:
    ErrorScreen(const String& message);

    void draw(TFT_eSPI& tft) override;
    void update(TFT_eSPI& tft) override;

private:
    String message;
};

#endif //ERRORSCREEN_H
