#ifndef SCREEN_H
#define SCREEN_H

#include <lvgl.h>

#include "utils/Macros.h"

class Screen {
public:
    virtual ~Screen() {
        LOG_DEBUG("Deleting screen '%s' (%p)", lv_obj_get_name(_screen), _screen);
        lv_obj_delete_async(_screen); // TODO: fix Screen deletion
    }
    virtual void update() {}
    lv_obj_t* root() const { return _screen; }
protected:
    lv_obj_t *_screen = nullptr;
};

#endif //SCREEN_H