#include "LvglTask.h"
#include <lvgl.h>

LvglTask::LvglTask() : Task("LvglTask", 8192, 1) {}

[[noreturn]] void LvglTask::run() {
    for (;;) {
        lv_timer_handler();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}
