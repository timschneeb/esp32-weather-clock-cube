#ifndef DISPLAYSERVICE_H
#define DISPLAYSERVICE_H

#include <lvgl.h>
#include <memory>

#include "Task.h"
#include "hardware/Backlight.h"
#include "hardware/Button.h"
#include "hardware/TFT.h"
#include "services/display/Screen.h"
#include "utils/Macros.h"

#define DISP_PANIC(msg) DisplayService::instance().panic(msg, __func__, __LINE__, __FILE__);

class DisplayService final : public Task {
    SINGLETON(DisplayService)

public:
    [[noreturn]] void panic(const char* msg, const char* func, int line, const char* file);
    void setScreen(std::unique_ptr<Screen> newScreen, unsigned long timeoutSec = 0);
    void showOverlay(const String& message, unsigned long duration);
    void pushToDisplay(uint16_t* buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

protected:
    [[noreturn]] void run() override;

private:
    static void deleteObjectOnTimer(lv_timer_t* timer);
    void displayImageFromAPI(const String &url, const String &zone);

    Button button;
    Backlight backlight;
    TFT tft;

    lv_display_t *disp;
    lv_draw_buf_t drawBuffer;
    uint16_t *drawBuf1;
    uint16_t *drawBuf2;

    std::unique_ptr<Screen> currentScreen;
    
    unsigned long lastKeepaliveTime = 0;
};

#endif //DISPLAYSERVICE_H
