//
// Created by tim on 24.09.25.
//

#include "Settings.h"

Settings::Settings() {
    load();
}

void Settings::load() {
    preferences.begin("config", true);
#define L_STRING(name, def) name.store(preferences.getString(#name, def));
#define L_INT(name, def) name.store(preferences.getInt(#name, def));
#define L_FLOAT(name, def) name.store(preferences.getFloat(#name, def));
    FOR_EACH_PROPERTY(L_STRING, L_INT, L_FLOAT)
    preferences.end();
}

void Settings::save() {
    preferences.begin("config", false);
#define S_STRING(name, def) preferences.putString(#name, def);
#define S_INT(name, def) preferences.putInt(#name, def);
#define S_FLOAT(name, def) preferences.putFloat(#name, def);
    FOR_EACH_PROPERTY(S_STRING, S_INT, S_FLOAT)
    preferences.end();
}
