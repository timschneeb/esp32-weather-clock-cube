//
// Created by tim on 24.09.25.
//

#ifndef NETWORK_H
#define NETWORK_H

#include <QuarkTS.h>

#include "utils/Macros.h"

class NetworkService final : public qOS::task {
    SINGLETON(NetworkService)
public:
    static void registerTask();

    void connectToSavedWiFi();

    static String getSavedSSID();
    static bool isConnected();

protected:
    void activities(qOS::event_t e) override;

private:
    static void enterAPMode();
    static bool isInApMode();
};



#endif //NETWORK_H
