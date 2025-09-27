#include "Backlight.h"

#include "Config.h"
#include <Arduino.h>
#include "Settings.h"

Backlight::Backlight() {
    ledcSetup(TFT_BL_PWM_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, TFT_BL_PWM_CHANNEL);
    setBrightness(0);
}

void Backlight::setBrightness(int brt) const {
    if (isManuallySleeping || isAutomaticallySleeping) {
        brt = 0;
    }
    brt = constrain(brt, 0, 255);
    ledcWrite(TFT_BL_PWM_CHANNEL, 255 - brt);
}

void Backlight::sleep() {
    isAutomaticallySleeping = true;
    setBrightness(0);
}

void Backlight::wake() {
    isAutomaticallySleeping = false;
    isManuallySleeping = false;
    setBrightness(Settings::instance().brightness);
}

void Backlight::handlePowerButton() {
    isManuallySleeping = !isManuallySleeping;
    isAutomaticallySleeping = false;
    setBrightness(Settings::instance().brightness);
}

bool Backlight::isSleeping() const {
    return isManuallySleeping || isAutomaticallySleeping;
}

bool Backlight::isSleepingByPowerButton() const {
    return isManuallySleeping;
}
