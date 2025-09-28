#include "EventBus.h"
#include <Arduino.h> // For Serial

#include "services/DisplayService.h"

EventBus::EventBus(): subscriptions() {
    mutex = xSemaphoreCreateMutex();
}

EventBus::~EventBus()
{
    vSemaphoreDelete(mutex);
}

void EventBus::subscribe(const EventId eventId, const QueueHandle_t queue)
{
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        if (subscriberCount < EVENTBUS_MAX_SUBSCRIBERS) {
            subscriptions[subscriberCount] = { eventId, queue };
            subscriberCount++;
        } else {
            Serial.println("[EventBus] ERROR: Max subscribers reached. Subscription failed.");
        }
        xSemaphoreGive(mutex);
    }
}

void EventBus::unsubscribe(const EventId eventId, const QueueHandle_t queue)
{
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        for (size_t i = 0; i < subscriberCount; ++i) {
            if (subscriptions[i].eventId == eventId && subscriptions[i].queue == queue) {
                // Found the subscriber, remove it by swapping with the last element
                subscriptions[i] = subscriptions[subscriberCount - 1];
                subscriberCount--;
                break; // Exit after removing the first match
            }
        }
        xSemaphoreGive(mutex);
    }
}

void EventBus::publish(const EventPtr &event, const TickType_t ticksToWait, const bool urgent) const {
    if (!event) {
        Serial.println("[EventBus] ERROR: Event is null.");
        return;
    }

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("[EventBus] Publishing " + String(named_enum::name(event->id())) + " " + (urgent ? "(urgent)" : ""));
        for (size_t i = 0; i < subscriberCount; ++i) {
            const auto& sub = subscriptions[i];
            if (sub.eventId == event->id()) {
                auto* heapEventPtr = new EventPtr(event);
                Serial.println("[EventBus]\tto subscriber " + String(i));

                BaseType_t result;
                if (urgent)
                    result = xQueueSendToFront(sub.queue, &heapEventPtr, ticksToWait);
                else
                    result = xQueueSend(sub.queue, &heapEventPtr, ticksToWait);

                if (result == errQUEUE_FULL) {
                    Serial.println("[EventBus]\tQueue full, event dropped for subscriber " + String(i));
                    DISP_PANIC(String(
                        "Queue full\n" +
                        String(named_enum::name(event->id())) + " dropped\n"
                        "Subscriber " + String(i) + "\n"
                        "Wait count: " + String(uxQueueMessagesWaiting(sub.queue))
                        ).c_str());
                }
            }
        }
        xSemaphoreGive(mutex);
    }
}

EventPtr EventBus::tryReceive(const QueueHandle_t queue, const TickType_t ticksToWait)
{
    EventPtr* heapEventPtr = nullptr;
    if (xQueueReceive(queue, &heapEventPtr, ticksToWait) == pdPASS) {
        EventPtr event = *heapEventPtr;
        delete heapEventPtr;
        return std::move(event);
    }
    return nullptr;
}

