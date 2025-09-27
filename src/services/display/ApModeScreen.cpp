#include "ApModeScreen.h"

#include <WiFi.h>

#include "Config.h"

void ApModeScreen::draw(TFT_eSPI& tft) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 30);
    tft.println("WiFi not connected");
    tft.setTextSize(3);
    tft.setCursor(20, 60);
    tft.println("**AP MODE**");
    tft.setTextSize(2);
    tft.setCursor(10, 110);
    tft.println("SSID: " + String(DEFAULT_SSID));
    tft.setCursor(10, 140);
    tft.println("PWD: " + String(DEFAULT_PASSWORD));
    tft.setCursor(10, 170);
    tft.println("IP: " + WiFi.softAPIP().toString());
}

void ApModeScreen::update(TFT_eSPI& tft) {
    // No update needed for error screen
}
