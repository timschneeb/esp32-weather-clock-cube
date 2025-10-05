#include "LvglDisplayAdapter.h"

#include <atomic>
#include <gui.h>
#include <lvgl.h>

#include "LvSPIFFS.h"
#include "services/DisplayService.h"

static OnFlushCallback onFlushCallback;
static std::atomic<bool> killed;

LvglDisplayAdapter::LvglDisplayAdapter() : display(nullptr), drawBuf1(nullptr), drawBuf2(nullptr) {}

void LvglDisplayAdapter::init(const uint32_t width, const uint32_t height) {
    lv_log_register_print_cb(onLog);

    lv_init();
    lv_tick_set_cb(getTicks);
    lv_fs_spiffs_init();

    display = lv_display_create(width, height);
    lv_display_set_flush_cb(display, onFlush);

    constexpr uint32_t bufferSize = 240 * 240 * sizeof(uint16_t);
    drawBuf1 = static_cast<uint16_t *>(heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM));
    drawBuf2 = static_cast<uint16_t *>(heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM));
    ASSERT_OR_PANIC(drawBuf1, "LVGL buffer 1 alloc failed");
    ASSERT_OR_PANIC(drawBuf2, "LVGL buffer 2 alloc failed");

    lv_display_set_buffers(display, drawBuf1, drawBuf2, bufferSize, LV_DISPLAY_RENDER_MODE_FULL);
    gui_init("S:/");
}

void LvglDisplayAdapter::setOnFlushCallback(const OnFlushCallback& callback) {
    onFlushCallback = callback;
}

void LvglDisplayAdapter::tick() {
    lv_timer_handler();
}

void LvglDisplayAdapter::panic() {
    killed.store(true);
}

uint32_t LvglDisplayAdapter::getTicks() {
    return xTaskGetTickCount() / portTICK_PERIOD_MS;
}

void LvglDisplayAdapter::onFlush(lv_display_t * disp, const lv_area_t * area, uint8_t * data) {
    if (killed == true) {
        // If we triggered a panic, we stop writing to the display.
        // The panic screen will be drawn using TFT_eSPI directly.
        // We do not call lv_display_flush_ready(), which will block LVGL flush task forever.
        return;
    }

    const uint32_t w = area->x2 - area->x1 + 1;
    const uint32_t h = area->y2 - area->y1 + 1;
    onFlushCallback(reinterpret_cast<uint16_t *>(data), area->x1, area->y1, w, h);
    lv_display_flush_ready(disp);
}

void LvglDisplayAdapter::onLog(const lv_log_level_t level, const char *buf) {
    switch (level) {
        case LV_LOG_LEVEL_ERROR:
            LOG_ERROR("%s", buf);
            break;
        case LV_LOG_LEVEL_WARN:
            LOG_WARN("%s", buf);
            break;
        case LV_LOG_LEVEL_INFO:
            LOG_INFO("%s", buf);
            break;
        case LV_LOG_LEVEL_TRACE:
            LOG_VERBOSE("%s", buf);
            break;
        default:
            LOG_DEBUG("%s", buf);
            break;
    }
}
