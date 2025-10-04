#ifndef IMAGESCREEN_H
#define IMAGESCREEN_H

#include "Screen.h"

#include <Arduino.h>
#include <lvgl.h>

class ImageScreen final : public Screen {
public:
    explicit ImageScreen(const String& filename);

private:
    String filename;
};

#endif //IMAGESCREEN_H