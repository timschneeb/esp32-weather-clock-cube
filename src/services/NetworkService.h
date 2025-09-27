//
// Created by tim on 24.09.25.
//

#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

#include "Task.h"
#include "utils/Macros.h"

class NetworkService final : public Task {
    SINGLETON(NetworkService)
public:
    static String getSavedSSID();
    static bool isConnected();
    static bool isInApMode();

protected:
    [[noreturn]] void run() override;

private:
    static void enterAPMode();
};



#endif //NETWORK_H
