//
// Created by tim on 27.09.25.
//

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <ArduinoJson.h>

class Diagnostics {
public:
    static void printTasks();
    static JsonDocument getTasksJson(bool print = false);
};



#endif //DIAGNOSTICS_H
