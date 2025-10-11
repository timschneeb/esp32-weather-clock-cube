//
// Created by tim on 29.09.25.
//

#ifndef ESP32_CUBE_DEBUGSERVICE_H
#define ESP32_CUBE_DEBUGSERVICE_H

#include "Task.h"
#include "utils/Macros.h"

class DebugService final : public Task {
    SINGLETON(DebugService)
protected:
    [[noreturn]] void run() override;
};



#endif //ESP32_CUBE_DEBUGSERVICE_H