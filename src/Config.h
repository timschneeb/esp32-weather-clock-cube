//
// Created by tim on 24.09.25.
//

#ifndef CONFIG_H
#define CONFIG_H

// Touch button
#define TOUCH_PIN 9
#define TOUCH_THRESHOLD 1500
#define TOUCH_LONG_PRESS_MS 500

// Backlight configuration
#define TFT_BL 14
#define TFT_BL_PWM_CHANNEL 0

// AP WiFi mode
#define DEFAULT_SSID "ESP32_AP"
#define DEFAULT_PASSWORD "admin1234"

// Timeouts
#define KEEPALIVE_TIMEOUT (5 * 60 * 1000) /* 5 minutes */



#endif //CONFIG_H
