#ifndef IMAGESCREEN_H
#define IMAGESCREEN_H

#include "Screen.h"

class ImageScreen final : public Screen {
public:
    ImageScreen(const String& filename);

    void draw(TFT_eSPI& tft) override;
    void update(TFT_eSPI& tft) override;

private:
    String filename;
};

#endif //IMAGESCREEN_H
