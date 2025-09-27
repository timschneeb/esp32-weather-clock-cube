#include "ErrorScreen.h"

ErrorScreen::ErrorScreen(const String& message) : message(message) {}

void ErrorScreen::draw(TFT_eSPI& tft) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 40);
    tft.println("Error:");
    tft.setCursor(10, 70);
    tft.println(message);
}

void ErrorScreen::update(TFT_eSPI& tft) {
    // No update needed for error screen
}
