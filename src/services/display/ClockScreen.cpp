#include "ClockScreen.h"

#include <TJpg_Decoder.h>

#include "Settings.h"

const char *daysShort[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *months[] = {
    "Jan.", "Feb.", "March", "April", "May", "June", "July", "August", "Sept.", "Oct.", "Nov.", "Dec."
};

void ClockScreen::draw(TFT_eSPI& tft) {
    tft.fillScreen(TFT_BLACK);
    lastDrawnWeatherIcon = "";
    lastDate = "";
    lastClockUpdate = 0;
    update(tft);
}

void ClockScreen::update(TFT_eSPI& tft) {
    time_t now = time(nullptr);
    tm *tm_info = localtime(&now);
    if (!tm_info) return;

    // Date
    String enDate = String(daysShort[tm_info->tm_wday]) + " " +
                    String(tm_info->tm_mday) + " " +
                    String(months[tm_info->tm_mon]);
    if (enDate != lastDate) {
        tft.fillRect(0, 0, 240, 25, TFT_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int dateWidth = tft.textWidth(enDate);
        int dateX = (240 - dateWidth) / 2;
        tft.setCursor(dateX, 0);
        tft.println(enDate);
        lastDate = enDate;
    }

    // Time
    if (millis() - lastClockUpdate > 1000) {
        lastClockUpdate = millis();
        char timeBuffer[20];
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", tm_info);
        tft.setTextSize(5);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int timeWidth = tft.textWidth(timeBuffer);
        int timeX = (240 - timeWidth) / 2;
        tft.setCursor(timeX, 45);
        tft.println(timeBuffer);
    }

    // Temperature
    auto tempValue = String(Settings::instance().weatherTempDay.load(), 1);
    String tempUnit = "÷c";
    auto humidityValue = String(static_cast<int>(Settings::instance().weatherHumidity));
    String humidityUnit = "%";
    auto tempMinValue = String(Settings::instance().weatherTempMin, 1);
    String tempMinUnit = "÷c";
    auto tempMaxValue = String(Settings::instance().weatherTempMax, 1);
    String tempMaxUnit = "÷c";

    tft.setTextSize(4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 105);
    tft.print(tempValue);
    int tempValueWidth = tft.textWidth(tempValue);
    tft.setTextSize(2);
    tft.setCursor(10 + tempValueWidth, 105);
    tft.print(tempUnit);
    int tempUnitWidth = tft.textWidth(tempUnit);
    tft.setTextSize(4);
    tft.setCursor(10 + tempValueWidth + tempUnitWidth + 5, 105);
    tft.print(humidityValue);
    int humidityValueWidth = tft.textWidth(humidityValue);
    tft.setTextSize(3);
    tft.setCursor(10 + tempValueWidth + tempUnitWidth + 5 + humidityValueWidth, 105);
    tft.print(humidityUnit);

    // Min label
    tft.setTextSize(2);
    tft.setCursor(2, 170);
    tft.print("Min ");
    int minLabelWidth = tft.textWidth("Min ");
    tft.setTextSize(3);
    tft.setCursor(2 + minLabelWidth, 165);
    tft.print(tempMinValue);
    int tempMinValueWidth = tft.textWidth(tempMinValue);
    tft.setTextSize(2);
    tft.setCursor(2 + minLabelWidth + tempMinValueWidth, 170);
    tft.print(tempMinUnit);

    // Max label
    tft.setTextSize(2);
    tft.setCursor(2, 200);
    tft.print("Max ");
    int maxLabelWidth = tft.textWidth("Max ");
    tft.setTextSize(3);
    tft.setCursor(2 + maxLabelWidth, 195);
    tft.print(tempMaxValue);
    int tempMaxValueWidth = tft.textWidth(tempMaxValue);
    tft.setTextSize(2);
    tft.setCursor(2 + maxLabelWidth + tempMaxValueWidth, 200);
    tft.print(tempMaxUnit);


    String weatherIcon = Settings::instance().weatherIcon.load();

    // Weather icon
    if (weatherIcon != lastDrawnWeatherIcon) {
        showWeatherIconJPG(tft, weatherIcon);
        lastDrawnWeatherIcon = weatherIcon;
    }
}

void ClockScreen::showWeatherIconJPG(TFT_eSPI& tft, const String& iconCode) {
    const String path = "/icons/" + iconCode + ".jpg";
    Serial.print("Search icon: ");
    Serial.println(path);
    constexpr int iconWidth = 90;
    constexpr int iconHeight = 90;
    constexpr int x = 240 - iconWidth - 8;
    constexpr int y = 240 - iconHeight - 8;
    if (SPIFFS.exists(path)) {
        TJpgDec.drawJpg(x, y, path.c_str());
        Serial.print("[WEATHER] Icon drawn: ");
        Serial.println(path);
    } else {
        Serial.print("[WEATHER] Icon NOT found: ");
        Serial.println(path);
        constexpr int pad = 10;
        tft.drawLine(x + pad, y + pad, x + iconWidth - pad, y + iconHeight - pad, TFT_RED);
        tft.drawLine(x + iconWidth - pad, y + pad, x + pad, y + iconHeight - pad, TFT_RED);
        tft.drawRect(x, y, iconWidth, iconHeight, TFT_RED);
    }
}
