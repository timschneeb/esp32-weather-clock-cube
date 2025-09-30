//
// Created by tim on 25.09.25.
//

#ifndef MACROS_H
#define MACROS_H

#include <Esp.h>
#include <freertos/task.h>

#define SINGLETON(name) \
    public: \
        static name& instance() \
        { \
            static name instance; \
            return instance; \
        } \
        name(name const&) = delete; \
        void operator=(name const&)  = delete; \
    private: \
        name();

#ifdef GET_TASK_STACK_INFO
#define LOG_VERBOSE(TAG,format,...) ESP_LOGV (TAG,"[S:%d] " format, uxTaskGetStackHighWaterMark(NULL), ##__VA_ARGS__);
#define LOG_DEBUG(TAG,format,...) ESP_LOGD (TAG,"[S:%d] " format, uxTaskGetStackHighWaterMark(NULL), ##__VA_ARGS__);
#define LOG_INFO(TAG,format,...) ESP_LOGI (TAG,"[S:%d] " format, uxTaskGetStackHighWaterMark(NULL), ##__VA_ARGS__);
#define LOG_WARN(TAG,format,...) ESP_LOGW (TAG,"[S:%d] " format, uxTaskGetStackHighWaterMark(NULL), ##__VA_ARGS__);
#define LOG_ERROR(TAG,format,...) ESP_LOGE (TAG,"[S:%d] " format, uxTaskGetStackHighWaterMark(NULL), ##__VA_ARGS__);
#else
#define LOG_VERBOSE(format,...) ESP_LOGV (pcTaskGetName(NULL), format, ##__VA_ARGS__);
#define LOG_DEBUG(format,...) ESP_LOGD (pcTaskGetName(NULL), format, ##__VA_ARGS__);
#define LOG_INFO(format,...) ESP_LOGI (pcTaskGetName(NULL), format, ##__VA_ARGS__);
#define LOG_WARN(format,...) ESP_LOGW (pcTaskGetName(NULL), format, ##__VA_ARGS__);
#define LOG_ERROR(format,...) ESP_LOGE (pcTaskGetName(NULL), format, ##__VA_ARGS__);
#endif // GET_TASK_STACK_INFO

// Note: Requires DisplayService include
#define ASSERT_OR_PANIC(x, msg) if(!(x)) { \
    LOG_ERROR("ASSERTION FAILED: '%s', %s", #x, #msg); \
    DISP_PANIC("ASSERTION FAILED\n\n" #x "\n" #msg); \
}

#define RETURN_IF_FAILED(x, msg) if(!(x)) { \
    LOG_ERROR("PRECONDITION FAILED: '%s', %s", #x, #msg); \
    return; \
}


#define DIAG_ENTER_SUPPRESS_IDLE_WDT \
    LOG_WARN("Suppressing idle WDT on core %d", xPortGetCoreID()); \
    if (xPortGetCoreID() == 0) \
        disableCore0WDT(); \
    else \
        disableCore1WDT();

#define DIAG_EXIT_SUPPRESS_IDLE_WDT \
    if (xPortGetCoreID() == 0) \
        enableCore0WDT(); \
    else \
        enableCore1WDT(); \
    LOG_WARN("Re-enabled idle WDT on core %d", xPortGetCoreID());

#endif //MACROS_H
