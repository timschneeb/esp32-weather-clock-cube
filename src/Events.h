//
// Created by tim on 24.09.25.
//

#ifndef EVENTS_H
#define EVENTS_H

#include "utils/EventMacros.h"
#include "AsyncMqttClient/DisconnectReasons.hpp"

enum class EventId {
    // NetworkService
    NET_ConnectStaNow,
    NET_Connecting,
    NET_StaConnected,
    NET_ApCreated,

    // WebServer
    API_KeepAlive,
    API_ShowImageFromUrl,
    WEB_MqttDisconnected,
    WEB_MqttError,
    WEB_ShowImageFromUrlWithZone,
    WEB_ShowLocalImage,

    // WeatherService
    WEA_ForecastUpdated,

    // Settings
    CFG_Updated,
    CFG_WeatherUpdated,
};


class IEvent {
public:
    EventId id() const { return _id; }

    template<typename T>
    T* to() {
        static_assert(std::is_base_of<IEvent, T>::value, "Type parameter must derive from EventArgs");
        return static_cast<T*>(this);
    }

protected:
    explicit IEvent(const EventId id) {
        _id = id;
    }

private:
    EventId _id;
};

REGISTER_EVENT_NOARGS(NET_ConnectStaNow);
REGISTER_EVENT_NOARGS(NET_Connecting);
REGISTER_EVENT_NOARGS(NET_StaConnected);
REGISTER_EVENT_NOARGS(NET_ApCreated);

REGISTER_EVENT(API_KeepAlive, (uint64_t, now));
REGISTER_EVENT(API_ShowImageFromUrl, (String, url));
REGISTER_EVENT(WEB_MqttDisconnected, (AsyncMqttClientDisconnectReason, reason));
REGISTER_EVENT(WEB_MqttError, (String, message));
REGISTER_EVENT(WEB_ShowImageFromUrlWithZone, (String, url), (String, zone));
REGISTER_EVENT(WEB_ShowLocalImage, (String, filename));

REGISTER_EVENT_NOARGS(WEA_ForecastUpdated);

REGISTER_EVENT_NOARGS(CFG_Updated);
REGISTER_EVENT_NOARGS(CFG_WeatherUpdated);

#endif //EVENTS_H
