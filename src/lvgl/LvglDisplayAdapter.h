//
// Created by tim on 29.09.25.
//

#ifndef ESP32_CUBE_LVGLDISPLAYADAPTER_H
#define ESP32_CUBE_LVGLDISPLAYADAPTER_H

#include <functional>
#include <lvgl.h>

typedef std::function<void(uint16_t* buffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h)> OnFlushCallback;

class LvglDisplayAdapter {
public:
    LvglDisplayAdapter();
    void init(uint32_t width, uint32_t height);
    void suspend() const;
    void resume() const;

    /// Kill & block LVGL flush task (on panic)
    static void panic();
    static void tick();
    static void setOnFlushCallback(const OnFlushCallback& callback);

private:
    lv_display_t* display;
    uint16_t *drawBuf1;
    uint16_t *drawBuf2;

    // LVGL callbacks
    static uint32_t getTicks();
    static void onFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *data);
    static void onLog(lv_log_level_t level, const char *buf);
};


#endif //ESP32_CUBE_LVGLDISPLAYADAPTER_H