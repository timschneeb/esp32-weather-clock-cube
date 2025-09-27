#ifndef DISPLAYSERVICE_H
#define DISPLAYSERVICE_H

#include "Task.h"
#include "utils/Macros.h"
#include <TFT_eSPI.h>
#include "hardware/Button.h"
#include "hardware/Backlight.h"
#include <vector>
#include <memory>
#include "display/Screen.h"
#include "display/ClockScreen.h"
#include "display/ErrorScreen.h"
#include "display/StatusScreen.h"
#include "display/ImageScreen.h"

#define DISP_PANIC(msg) DisplayService::instance().panic(msg, __func__, __LINE__, __FILE__);

class DisplayService final : public Task {
    SINGLETON(DisplayService)

public:
    void panic(const char* msg, const char* func, int line, const char* file);
    void setScreen(std::unique_ptr<Screen> newScreen, unsigned long timeoutSec = 0);
    void showOverlay(const String& message, unsigned long duration);

protected:
    [[noreturn]] void run() override;

private:
    bool jpgRenderCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
    void handleSlideshow();
    void displayImageFromAPI(const String &url, const String &zone);

    Button button;
    Backlight backlight;
    TFT_eSPI tft;

    std::unique_ptr<Screen> currentScreen;
    std::unique_ptr<StatusScreen> overlayScreen;
    
    unsigned long lastKeepaliveTime = 0;
    unsigned long screenTimeout = 0;
    unsigned long screenSince = 0;

    // --- Slideshow ---
    String pendingImageUrl = "";
    bool imagePending = false;
    String pendingZone = "";
    bool slideshowActive = false;
    unsigned long slideshowStart = 0;
    int currentSlideshowIdx = 0;
    std::vector<String> jpgQueue; // Array of full paths to JPG files
    std::vector<unsigned long> eventCallTimes;
    unsigned long lastEventCall = 0;
};

#endif //DISPLAYSERVICE_H
