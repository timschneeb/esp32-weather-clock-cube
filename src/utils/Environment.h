//
// Created by tim on 01.10.25.
//

#ifndef GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_ENVIRONMENT_H
#define GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_ENVIRONMENT_H

#include <Arduino.h>

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
}

#endif //GEEKMAGIC_S3_FRIGATE_EVENT_VIEWER_ENVIRONMENT_H