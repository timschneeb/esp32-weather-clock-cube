//
// Created by tim on 01.10.25.
//

#ifndef ESP32_CUBE_ENVIRONMENT_H
#define ESP32_CUBE_ENVIRONMENT_H

#include <WString.h>
#include <esp_sntp.h>

#include "utils/Macros.h"

namespace Environment {
    inline bool isWokwiEmulator() {
        static int _isWokwiCache = -1;
        if (_isWokwiCache != -1)
            return _isWokwiCache == 1;

        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);

        // Emulator MAC set in diagram.json: 24:0a:c4:12:34:56
        _isWokwiCache = mac[0] == 0x24 && mac[1] == 0x0a && mac[2] == 0xc4 ? 1 : 0;
        if (_isWokwiCache == 1)
            LOG_INFO("Detected Wokwi emulator environment");
        return _isWokwiCache;
    }

    inline void setTimezone(const String& timezone) {
        LOG_INFO("Setting Timezone to %s\n", timezone.c_str());
        setenv("TZ", timezone.c_str(), 1);
        tzset();
    }

    inline bool isTimeSynchronized() {
        return time(nullptr) >= 1000000000 && sntp_get_sync_status() != SNTP_SYNC_STATUS_IN_PROGRESS;
    }

    inline void setupSntp() {
        esp_netif_init();

        if (sntp_enabled()) {
            sntp_stop();
        }

        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, SNTP_SERVER);
        sntp_init();
    }
}

#endif //ESP32_CUBE_ENVIRONMENT_H