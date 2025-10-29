#ifndef DISPLAYSERVICE_H
#define DISPLAYSERVICE_H

#include <lvgl.h>
#include <memory>
#include <WString.h>

#include "Task.h"
#include "hardware/Backlight.h"
#include "hardware/Button.h"
#include "hardware/IDisplay.h"
#include "lvgl/LvglDisplayAdapter.h"
#include "screens/Screen.h"
#include "utils/Macros.h"
#include "utils/Power.h"

#define DISP_PANIC(msg) DisplayService::instance().panic(msg, __func__, __LINE__, __FILE__);

class DisplayService final : public Task<12288, Priority::Normal> {
    TASK(DisplayService)
public:
    ~DisplayService() override;
    [[noreturn]] void panic(const char* msg, const char* func, int line, const char* file);
    void changeScreen(std::unique_ptr<Screen> newScreen, unsigned long timeoutSec = 0);
    void showOverlay(const String& message, unsigned long duration);

protected:
    [[noreturn]] void run() override;

private:
    void onSleepStateChanged(bool sleeping) const;
    void displayImageFromAPI(const String &url);

    Power power;
    Button button;
    Backlight backlight;
    IDisplay* display;

    LvglDisplayAdapter lvglAdapter;

    std::unique_ptr<Screen> currentScreen;

    bool isWaitingForTimeSync = false;
    unsigned long lastKeepaliveTime = 0;
    unsigned long screenTimeout = 0;
    unsigned long screenSince = 0;
};

#endif //DISPLAYSERVICE_H
