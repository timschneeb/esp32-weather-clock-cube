#include "Backlight.h"

#include "../utils/Power.h"
#include "Settings.h"

#ifndef QEMU_EMULATION
#include <esp32-hal-ledc.h>
#include "Config.h"
#endif

Backlight::Backlight() {
#ifndef QEMU_EMULATION
    ledcSetup(TFT_BL_PWM_CHANNEL, 5000, 8);
    ledcAttachPin(TFT_BL, TFT_BL_PWM_CHANNEL);
#endif
    setBrightness(0);
}

void Backlight::setBrightness(int brt) const {
    if (Power::isSleeping()) {
        brt = 0;
    }
    brt = constrain(brt, 0, 255);
#ifndef QEMU_EMULATION
    ledcWrite(TFT_BL_PWM_CHANNEL, 255 - brt);
#else
    LOG_DEBUG("Backlight set to %d", brt);
#endif
}