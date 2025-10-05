#ifndef SCREEN_H
#define SCREEN_H

#include <lvgl.h>

class Screen {
public:
    virtual ~Screen() = default;
    virtual void update() {}
    lv_obj_t* root() const { return _screen; }
protected:
    lv_obj_t *_screen = nullptr;
};

#endif //SCREEN_H