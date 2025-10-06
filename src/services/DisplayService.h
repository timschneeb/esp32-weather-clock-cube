#ifndef DISPLAYSERVICE_H
#define DISPLAYSERVICE_H

#include <lvgl.h>
#include <memory>

#include "Task.h"
#include "hardware/Backlight.h"
#include "hardware/Button.h"
#include "hardware/TFT.h"
#include "lvgl/LvglDisplayAdapter.h"
#include "../screens/Screen.h"
#include "utils/Macros.h"

#define DISP_PANIC(msg) DisplayService::instance().panic(msg, __func__, __LINE__, __FILE__);

class DisplayService final : public Task {
    SINGLETON(DisplayService)

public:
    [[noreturn]] void panic(const char* msg, const char* func, int line, const char* file);
    void changeScreen(std::unique_ptr<Screen> newScreen, unsigned long timeoutSec = 0);
    void showOverlay(const String& message, unsigned long duration);

protected:
    [[noreturn]] void run() override;

private:
    void displayImageFromAPI(const String &url);

    Button button;
    Backlight backlight;
    TFT tft;

    LvglDisplayAdapter lvglAdapter;

    std::unique_ptr<Screen> currentScreen;

    bool isWaitingForTimeSync = false;
    unsigned long lastKeepaliveTime = 0;
    unsigned long screenTimeout = 0;
    unsigned long screenSince = 0;
};

#endif //DISPLAYSERVICE_H
