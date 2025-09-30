//
// Created by tim on 27.09.25.
//

#include "Diagnostics.h"

#include "Macros.h"

void Diagnostics::printTasks() {
    getTasksJson(true);
}

JsonDocument Diagnostics::getTasksJson(const bool print) {
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
            LOG_INFO("==> Active tasks (runtime: %ul ticks)", ulTotalRunTime);
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

            // TODO add more; like runtime percentage
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
