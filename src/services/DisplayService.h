#ifndef DISPLAYSERVICE_H
#define DISPLAYSERVICE_H

#include "Task.h"
#include "utils/Macros.h"
#include <TFT_eSPI.h>
#include "hardware/Button.h"
#include "hardware/Backlight.h"
#include <vector>

#define DISP_PANIC(msg) DisplayService::instance().panic(msg, __func__, __LINE__, __FILE__);

class DisplayService final : public Task {
    SINGLETON(DisplayService)

public:
    void panic(const char* msg, const char* func, int line, const char* file);

protected:
    [[noreturn]] void run() override;

private:
    void setScreen(const String &newScreen, unsigned long timeoutSec = 0, const char *by = "");
    bool jpgRenderCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
    void handleSlideshow();
    void showWeatherIconJPG(const String& iconCode);
    void showClock();
    void displayImageFromAPI(const String &url, const String &zone);

    // Variables
    String lastDrawnWeatherIcon = "";
    String lastDate = "";
    String currentScreen = "clock"; // ["clock", "event", "status", "error"]

    Button button;
    Backlight backlight;
    TFT_eSPI tft;

    unsigned long lastClockUpdate = 0;
    unsigned long lastKeyTime = 0;
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
