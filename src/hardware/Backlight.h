#ifndef BACKLIGHT_H
#define BACKLIGHT_H

class Backlight {
public:
    Backlight();

    void setBrightness(int brt) const;
    void sleep();
    void wake();
    void handlePowerButton();

    bool isSleeping() const;
    bool isSleepingByPowerButton() const;

private:
    bool isManuallySleeping = false;
    bool isAutomaticallySleeping = false;
};

#endif // BACKLIGHT_H
