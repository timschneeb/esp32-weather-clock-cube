#ifndef IMAGESCREEN_H
#define IMAGESCREEN_H

#include "Screen.h"

#include <Arduino.h>
#include <lvgl.h>

class ImageScreen final : public Screen {
public:
    explicit ImageScreen(const String& filename, bool deleteLater = false);
    ~ImageScreen() override;

private:
    String filename;
    bool deleteLater;
};

#endif //IMAGESCREEN_H