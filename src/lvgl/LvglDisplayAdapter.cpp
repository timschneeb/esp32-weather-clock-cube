#include "LvglDisplayAdapter.h"

#include <lvgl.h>

#include "LvSPIFFS.h"
#include "services/DisplayService.h"

static OnFlushCallback onFlushCallback;

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
    if (!drawBuf1 || !drawBuf2) {
        Serial.println("Failed to allocate LVGL draw buffers");
        DISP_PANIC("LVGL buffer alloc failed");
    }

    lv_display_set_buffers(display, drawBuf1, drawBuf2, bufferSize, LV_DISPLAY_RENDER_MODE_FULL);
}

void LvglDisplayAdapter::setOnFlushCallback(const OnFlushCallback& callback) {
    onFlushCallback = callback;
}

void LvglDisplayAdapter::tick() {
    lv_timer_handler();
}

uint32_t LvglDisplayAdapter::getTicks() {
    return xTaskGetTickCount() / portTICK_PERIOD_MS;
}

void LvglDisplayAdapter::onFlush(lv_display_t * disp, const lv_area_t * area, uint8_t * data) {
    const uint32_t w = area->x2 - area->x1 + 1;
    const uint32_t h = area->y2 - area->y1 + 1;
    onFlushCallback(reinterpret_cast<uint16_t *>(data), area->x1, area->y1, w, h);
    lv_display_flush_ready(disp);
}

void LvglDisplayAdapter::onLog(lv_log_level_t level, const char *buf) {
    // TODO create logging system and integrate with this
    Serial.write(buf);
}
