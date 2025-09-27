#include "StatusScreen.h"

#include <Arduino.h>

StatusScreen::StatusScreen(const String& message, const unsigned long timeout)
    : message(message), startTime(millis()), timeout(timeout) {}

void StatusScreen::draw(TFT_eSPI& tft) {
    tft.fillRect(0, 200, 240, 40, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 210);
    tft.println(message);
}

void StatusScreen::update(TFT_eSPI& tft) {
    // This screen is static, but we could add animations or updates here
}

bool StatusScreen::isExpired() const {
    return millis() - startTime > timeout;
}
