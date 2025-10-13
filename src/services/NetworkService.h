//
// Created by tim on 24.09.25.
//

#ifndef NETWORK_H
#define NETWORK_H

#include <WString.h>
#include <esp_event.h>

#include "Task.h"
#include "utils/Macros.h"

class NetworkService final : public Task<4096, Priority::Background> {
    TASK_NO_CTOR(NetworkService)
public:
    static bool isConnected();
    static bool isInApMode();
    static String getApIpString();
    static String getStaIpString();

protected:
    [[noreturn]] void run() override;

private:
    static void enterAPMode();
    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static bool s_isConnected;
    static bool s_isInApMode;
};



#endif //NETWORK_H
