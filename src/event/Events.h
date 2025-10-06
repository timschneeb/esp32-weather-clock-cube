//
// Created by tim on 24.09.25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include <Arduino.h>
#include <type_traits>

#include "AsyncMqttClient/DisconnectReasons.hpp"
#include "utils/NamedEnum.h"
#include "utils/EventMacros.h"

MAKE_NAMED_ENUM_CLASS(EventId,
    /* NetworkService */
    NET_Connecting,
    NET_StaConnected,
    NET_ApCreated,

    /* WebServer */
    API_KeepAlive,
    API_ShowImageFromUrl,
    WEB_MqttDisconnected,
    WEB_MqttError,
    WEB_ShowLocalImage,

    /* WeatherService */
    WEA_ForecastUpdated,

    /* Settings */
    CFG_Updated,
    CFG_BrightnessUpdated
);

class IEvent {
public:
    EventId id() const { return _id; }

    template<typename T>
    T* to() {
        static_assert(std::is_base_of<IEvent, T>::value, "Type parameter must derive from EventArgs");
        return static_cast<T*>(this);
    }

protected:
    explicit IEvent(const EventId id) : _id(id) {}

private:
    EventId _id;
};

REGISTER_EVENT_NOARGS(NET_Connecting);
REGISTER_EVENT_NOARGS(NET_StaConnected);
REGISTER_EVENT_NOARGS(NET_ApCreated);

REGISTER_EVENT(API_KeepAlive, (unsigned long, now));
REGISTER_EVENT(API_ShowImageFromUrl, (String, url));
REGISTER_EVENT(WEB_MqttDisconnected, (AsyncMqttClientDisconnectReason, reason));
REGISTER_EVENT(WEB_MqttError, (String, message));
REGISTER_EVENT(WEB_ShowLocalImage, (String, filename));

REGISTER_EVENT_NOARGS(WEA_ForecastUpdated);

REGISTER_EVENT_NOARGS(CFG_Updated);
REGISTER_EVENT(CFG_BrightnessUpdated, (int, brightness));

#endif //EVENTS_H
