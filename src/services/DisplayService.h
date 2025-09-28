#ifndef DISPLAYSERVICE_H
#define DISPLAYSERVICE_H

#include "Task.h"
#include "utils/Macros.h"
#include <lvgl.h>
#include "hardware/Button.h"
#include "hardware/Backlight.h"
#include <vector>
#include <memory>
#include "services/display/Screen.h"

#define DISP_PANIC(msg) DisplayService::instance().panic(msg, __func__, __LINE__, __FILE__);

class DisplayService final : public Task {
    SINGLETON(DisplayService)

public:
    void panic(const char* msg, const char* func, int line, const char* file);
    void setScreen(std::unique_ptr<Screen> newScreen, unsigned long timeoutSec = 0);
    void showOverlay(const String& message, unsigned long duration);

protected:
    [[noreturn]] void run() override;

private:
    void displayImageFromAPI(const String &url, const String &zone);

    Button button;
    Backlight backlight;

    lv_disp_drv_t disp_drv;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t *buf1;
    lv_color_t *buf2;

    std::unique_ptr<Screen> currentScreen;
    
    unsigned long lastKeepaliveTime = 0;
};

#endif //DISPLAYSERVICE_H
