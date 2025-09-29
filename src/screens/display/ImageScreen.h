#ifndef IMAGESCREEN_H
#define IMAGESCREEN_H

#include "Screen.h"
#include <Arduino.h>

class ImageScreen final : public Screen {
public:
    explicit ImageScreen(const String& filename);
    void draw(lv_obj_t* screen) override;

private:
    String filename;
};

#endif //IMAGESCREEN_H