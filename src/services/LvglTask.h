#ifndef LVGLTASK_H
#define LVGLTASK_H

#include "services/Task.h"

class LvglTask final : public Task {
public:
    LvglTask();

protected:
    [[noreturn]] void run() override;
};

#endif //LVGLTASK_H
