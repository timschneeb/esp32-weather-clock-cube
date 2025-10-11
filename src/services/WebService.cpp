//
// Created by tim on 24.09.25.
//

#include "WebService.h"

#include <ArduinoJson.h>
#include <Update.h>

#include "NetworkService.h"

using namespace ArduinoJson;

WebService::WebService(): Task("WebServices", 4096, 1) {

}

[[noreturn]] void WebService::run()
{
    for (;;) {
        api.tick();
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
