//
// Created by tim on 29.09.25.
//

#include "DebugService.h"

#include "utils/Diagnostics.h"

#define DEBUG_DUMP_INTERVAL_MS 10000

DebugService::DebugService() : Task("DebugService", 4096, 1) {}

void DebugService::run() {
    for (;;) {
        /*Diagnostics::printHeapUsage();
        Diagnostics::printGlobalHeapWatermark();
        Diagnostics::printTasks();*/
        vTaskDelay(pdMS_TO_TICKS(DEBUG_DUMP_INTERVAL_MS));
    }
}
