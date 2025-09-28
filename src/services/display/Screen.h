#ifndef SCREEN_H
#define SCREEN_H

#include <lvgl.h>

class Screen {
public:
    virtual ~Screen() = default;
    virtual void draw(lv_obj_t* screen) = 0;
    virtual void update() {};
protected:
    lv_obj_t* _screen;
};

#endif //SCREEN_H