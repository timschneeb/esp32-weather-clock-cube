//
// Created by tim on 29.10.25.
//

#include "Power.h"

#include <atomic>

static std::atomic<bool> isManuallySleeping;
static std::atomic<bool> isAutomaticallySleeping;

void Power::setOnSleepStateChanged(const std::function<void(bool)> &callback) {
    onSleepStateChanged = callback;
}

void Power::sleep() const {
    isAutomaticallySleeping = true;
    onSleepStateChanged(true);
}

void Power::wake() const {
    isAutomaticallySleeping = false;
    isManuallySleeping = false;
    onSleepStateChanged(false);
}

void Power::toggleManualSleep() const {
    isManuallySleeping = !isManuallySleeping;
    isAutomaticallySleeping = false;
    onSleepStateChanged(isManuallySleeping);
}

bool Power::isSleeping() {
    return isManuallySleeping || isAutomaticallySleeping;
}

bool Power::isSleepingByPowerButton() {
    return isManuallySleeping;
}
