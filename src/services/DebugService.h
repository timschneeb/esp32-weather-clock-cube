//
// Created by tim on 29.09.25.
//

#ifndef GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_DEBUGSERVICE_H
#define GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_DEBUGSERVICE_H

#include "Task.h"
#include "utils/Macros.h"

class DebugService final : public Task {
    SINGLETON(DebugService)
protected:
    [[noreturn]] void run() override;
};



#endif //GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_DEBUGSERVICE_H