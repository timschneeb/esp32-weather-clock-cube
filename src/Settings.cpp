//
// Created by tim on 24.09.25.
//

#include "Settings.h"

#include <Preferences.h>

Settings::Settings() : preferences(new Preferences()) {
    load();
}

Settings::~Settings() {
    delete preferences;
}

void Settings::load() {
    preferences->begin("config", true);
#define L_STRING(name, def) name.store(preferences->getString(#name, def));
#define L_INT(name, def) name.store(preferences->getInt(#name, def));
#define L_FLOAT(name, def) name.store(preferences->getFloat(#name, def));
    FOR_EACH_PROPERTY(L_STRING, L_INT, L_FLOAT)
    preferences->end();
}

void Settings::save() const {
    preferences->begin("config", false);
#define S_STRING(name, def) preferences->putString(#name, name);
#define S_INT(name, def) preferences->putInt(#name, name);
#define S_FLOAT(name, def) preferences->putFloat(#name, name);
    FOR_EACH_PROPERTY(S_STRING, S_INT, S_FLOAT)
    preferences->end();
}
