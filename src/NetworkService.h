//
// Created by tim on 24.09.25.
//

#ifndef NETWORK_H
#define NETWORK_H



#include <Arduino.h>
#include "utils/Macros.h"

class NetworkService final {
    SINGLETON(NetworkService)
public:
    void connectToSavedWiFi();

    static String getSavedSSID();
    static bool isConnected();

    void run(void *pvParameters);

private:
    static void enterAPMode();
    static bool isInApMode();
};



#endif //NETWORK_H
