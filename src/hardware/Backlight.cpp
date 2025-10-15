#include "Backlight.h"

#ifndef QEMU_EMULATION
#include <esp32-hal-ledc.h>
#include "Config.h"
#endif

#include "Settings.h"

Backlight::Backlight() {
#ifndef QEMU_EMULATION
    ledcSetup(TFT_BL_PWM_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, TFT_BL_PWM_CHANNEL);
#endif
    setBrightness(0);
}

void Backlight::setBrightness(int brt) const {
    if (isManuallySleeping || isAutomaticallySleeping) {
        brt = 0;
    }
    brt = constrain(brt, 0, 255);
#ifndef QEMU_EMULATION
    ledcWrite(TFT_BL_PWM_CHANNEL, 255 - brt);
#else
    LOG_DEBUG("Backlight set to %d", brt);
#endif
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
