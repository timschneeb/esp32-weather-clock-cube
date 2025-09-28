#include "LvglTask.h"
#include <lvgl.h>

LvglTask::LvglTask() : Task("LvglTask", 8192, 2) {}

[[noreturn]] void LvglTask::run() {
    for (;;) {
        // TODO remove?
        vTaskDelay(lv_timer_handler() / portTICK_PERIOD_MS);
    }
}
