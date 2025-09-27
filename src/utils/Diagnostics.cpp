//
// Created by tim on 27.09.25.
//

#include "Diagnostics.h"

void Diagnostics::printTasks() {
    getTasksJson(true);
}

JsonDocument Diagnostics::getTasksJson(const bool print) {
    volatile UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
    auto doc = JsonDocument();
    const auto root = doc.to<JsonObject>();
    const auto tasks = root["tasks"].to<JsonArray>();
    const auto pxTaskStatusArray = static_cast<TaskStatus_t *>(pvPortMalloc(uxArraySize * sizeof(TaskStatus_t)));
    if (pxTaskStatusArray) {
        uint32_t ulTotalRunTime;
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        root["rt"] = ulTotalRunTime;

        if (print) {
            Serial.println("==> Active tasks (runtime: " + String(ulTotalRunTime) + " ticks)");
            Serial.println("\tNum\tState\tCurPrio\tBasePrio\tRunTimeCnt\tMaxStack\tName");
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

            if (print) {
                Serial.printf("\t%-2d\t%-5d\t%-7d\t%-9d\t%-11d\t%-8d\t%s\n",
                              tp->xTaskNumber,
                                tp->eCurrentState,
                                tp->uxCurrentPriority,
                                tp->uxBasePriority,
                                tp->ulRunTimeCounter,
                                tp->usStackHighWaterMark,
                                tp->pcTaskName);
            }
        }
        Serial.println();
    }
    vPortFree(pxTaskStatusArray);
    return std::move(doc);
}
