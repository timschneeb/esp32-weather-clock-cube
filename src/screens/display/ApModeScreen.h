#ifndef APMODESCREEN_H
#define APMODESCREEN_H

#include "Screen.h"

class ApModeScreen final : public Screen {
public:
    void draw(lv_obj_t* screen) override;
};

#endif //APMODESCREEN_H