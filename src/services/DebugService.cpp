//
// Created by tim on 29.09.25.
//

#include "DebugService.h"

#define DEBUG_DUMP_INTERVAL_MS 10000

DebugService::DebugService() : Task("DebugService", 2048, 1) {}

void DebugService::run() {
    for (;;) {

        vTaskDelay(pdMS_TO_TICKS(DEBUG_DUMP_INTERVAL_MS));
    }
}
