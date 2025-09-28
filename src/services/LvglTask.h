#ifndef LVGLTASK_H
#define LVGLTASK_H

#include "services/Task.h"
#include "utils/Macros.h"

class LvglTask final : public Task {
    SINGLETON(LvglTask)
protected:
    [[noreturn]] void run() override;
};

#endif //LVGLTASK_H
