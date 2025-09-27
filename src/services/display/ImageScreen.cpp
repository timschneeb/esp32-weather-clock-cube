#include "ImageScreen.h"

#include <TJpg_Decoder.h>
#include <SPIFFS.h>

ImageScreen::ImageScreen(const String& filename) : filename(filename) {}

void ImageScreen::draw(TFT_eSPI& tft) {
    if (SPIFFS.exists(filename)) {
        File file = SPIFFS.open(filename, FILE_READ);
        if (file) {
            const auto fileSize = file.size();
            auto *jpgData = static_cast<uint8_t *>(malloc(fileSize));
            if (jpgData != nullptr) {
                const size_t bytesRead = file.readBytes(reinterpret_cast<char *>(jpgData), fileSize);
                file.close();
                if (bytesRead == fileSize) {
                    tft.fillScreen(TFT_BLACK);
                    TJpgDec.drawJpg(0, 0, jpgData, fileSize);
                }
                free(jpgData);
            }
        }
    }
}

void ImageScreen::update(TFT_eSPI& tft) {
    // No update needed for image screen
}
