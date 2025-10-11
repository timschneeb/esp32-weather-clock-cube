//
// Created by tim on 24.09.25.
//

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <Arduino.h>

#include "Task.h"
#include "WebApi.h"
#include "utils/Macros.h"

class WebService final : public Task {
    SINGLETON(WebService)
public:

protected:
    [[noreturn]] void run() override;

private:
    WebApi api;

};

#endif // WEBINTERFACE_H
