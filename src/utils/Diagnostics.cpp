//
// Created by tim on 27.09.25.
//

#include "Diagnostics.h"

#include "Macros.h"

void Diagnostics::printFullHeapDump() {
    // Disable watchdogs to prevent resets during long debug operation
    DIAG_ENTER_SUPPRESS_IDLE_WDT
    heap_caps_dump_all();
    LOG_INFO()
    DIAG_EXIT_SUPPRESS_IDLE_WDT
}

void Diagnostics::printTasks() {
    getTasksJson(true);
}

void Diagnostics::printGlobalHeapWatermark() {
    LOG_INFO("Global heap high watermarks: SRAM %d bytes; PSRAM %d bytes",
        heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
}

void Diagnostics::printHeapUsage() {
    LOG_INFO("%s", collectHeapUsage().c_str());
}

void Diagnostics::printHeapUsageSafely() {
    const auto freeInternal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const auto lfbInternal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const auto totalInternal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    const auto freeSpi = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const auto lfbSpi = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    const auto totalSpi = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    LOG_INFO(
        "Heap usage:\n"
        "        Free%%   Biggest /     Free /    Total\n"
        " SRAM : %2d%%   [%8d / %8d / %8d] bytes\n"
        "PSRAM : %2d%%   [%8d / %8d / %8d] bytes",
        static_cast<int>(lfbInternal * 100 / totalInternal), lfbInternal, freeInternal, totalInternal,
        static_cast<int>(lfbSpi * 100 / totalSpi), lfbSpi, freeSpi, totalSpi
    );
}

String Diagnostics::collectHeapUsage() {
    #define PRINT_INFO_BUFFER_SIZE  256
    const auto freeInternal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const auto lfbInternal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const auto totalInternal = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    const auto freeSpi = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const auto lfbSpi = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    const auto totalSpi = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);

    const auto buffer = static_cast<char*>(calloc(1, PRINT_INFO_BUFFER_SIZE));
    snprintf(
        buffer, PRINT_INFO_BUFFER_SIZE,
        "Heap usage:\n"
        "        Free%%   Biggest /     Free /    Total\n"
        " SRAM : %2d%%   [%8d / %8d / %8d] bytes\n"
        "PSRAM : %2d%%   [%8d / %8d / %8d] bytes",
        static_cast<int>(lfbInternal * 100 / totalInternal), lfbInternal, freeInternal, totalInternal,
        static_cast<int>(lfbSpi * 100 / totalSpi), lfbSpi, freeSpi, totalSpi
    );
    const String result(buffer);
    free(buffer);
    return result;
}

JsonDocument Diagnostics::getTasksJson(const bool print) {
    // TODO: setup FreeRTOS correctly for this
#undef configUSE_TRACE_FACILITY
    return JsonDocument();
#ifndef configUSE_TRACE_FACILITY
    esp_system_abort("configUSE_TRACE_FACILITY must be set to 1 in sdkconfig to use Diagnostics::getTasksJson()");
#else
    volatile UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
    auto doc = JsonDocument();
    const auto root = doc.to<JsonObject>();
    const auto tasks = root["tasks"].to<JsonArray>();
    auto *const pxTaskStatusArray = static_cast<TaskStatus_t *>(pvPortMalloc(uxArraySize * sizeof(TaskStatus_t)));
    if (pxTaskStatusArray != nullptr) {
        uint32_t ulTotalRunTime;
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        root["rt"] = ulTotalRunTime;

        if (print) {
            LOG_INFO("==> Active tasks (runtime: %lu ticks)", ulTotalRunTime);
            LOG_INFO("\tNum\tState\tCurPrio\tBasePrio\tRunTimeCnt\tMaxStack\tName");
        }

        const TaskStatus_t *tp = pxTaskStatusArray;
        for (int i = 0; i < uxArraySize; i++) {
            auto ti = tasks.add<JsonObject>();
            ti["num"] = tp->xTaskNumber;
            ti["name"] = const_cast<char *>(tp->pcTaskName);
            ti["cur_state"] = tp->eCurrentState;
            ti["cur_prio"] = tp->uxCurrentPriority;
            ti["base_prio"] = tp->uxBasePriority;
            ti["runtime_cnt"] = tp->ulRunTimeCounter;
            ti["max_stack_usage"] = tp->usStackHighWaterMark;
            tp++;

            // TODO: add more; like runtime percentage
            if (print) {
                LOG_INFO("\t%-2d\t%-5d\t%-7d\t%-9d\t%-11d\t%-8d\t%s\n",
                        tp->xTaskNumber,
                        tp->eCurrentState,
                        tp->uxCurrentPriority,
                        tp->uxBasePriority,
                        tp->ulRunTimeCounter,
                        tp->usStackHighWaterMark,
                        tp->pcTaskName);
            }
        }
        LOG_INFO()
    }
    vPortFree(pxTaskStatusArray);
    return std::move(doc);
#endif
}
