//
// Created by tim on 27.09.25.
//

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <ArduinoJson.h>
/*
 * CONFIG_FREERTOS_USE_TRACE_FACILITY=y must be set in sdkconfig to use this
 * class. Additionally, CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y provides
 * CPU usage info
 */
class Diagnostics {
public:
    static void printFullHeapDump();
    static void printTasks();
    static void printGlobalHeapWatermark();
    static void printHeapUsage();
    static void printHeapUsageSafely();

    static String collectHeapUsage();

    static JsonDocument getTasksJson(bool print = false);
};



#endif //DIAGNOSTICS_H
