#ifndef STATUSSCREEN_H
#define STATUSSCREEN_H

#include "Screen.h"

class StatusScreen final : public Screen {
public:
    StatusScreen(const String& message, unsigned long timeout);

    void draw(TFT_eSPI& tft) override;
    void update(TFT_eSPI& tft) override;

    bool isExpired() const;

private:
    String message;
    unsigned long startTime;
    unsigned long timeout;
};

#endif //STATUSSCREEN_H
