//
// Created by tim on 29.09.25.
//

#include "TFT.h"

TFT::TFT() {
    tft.begin();
    tft.setRotation(0);
    tft.setSwapBytes(true);
}

void TFT::push(uint16_t *buffer, const uint32_t x, const uint32_t y, const uint32_t w, const uint32_t h) {
    tft.startWrite();
    tft.setAddrWindow(x, y, w, h);
    tft.pushColors(buffer, w * h, true);
    tft.endWrite();
}

void TFT::panic(const char *msg, const char *footer) {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.println("=== PANIC ===");
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.println();
    tft.println(msg);
    tft.println();
    tft.println(footer);
    tft.flush();
}
