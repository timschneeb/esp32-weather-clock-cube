#include "EventBus.h"
#include <Arduino.h> // For Serial

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

void EventBus::publish(const EventPtr &event, const TickType_t ticksToWait) const {
    if (!event) {
        return;
    }

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("[EventBus] Publishing event ID " + String(static_cast<int>(event->id())));
        for (size_t i = 0; i < subscriberCount; ++i) {
            const auto& sub = subscriptions[i];
            if (sub.eventId == event->id()) {
                auto* heapEventPtr = new EventPtr(event);
                Serial.println("[EventBus]\tto subscriber " + String(i) + "; ref_count_before_send: " + String(heapEventPtr->use_count()));
                xQueueSend(sub.queue, &heapEventPtr, ticksToWait);
                Serial.println("[EventBus]\tto subscriber " + String(i) + "; ref_count_after_send: " + String(heapEventPtr->use_count()));
            }
        }
        xSemaphoreGive(mutex);
    }
}

void EventBus::publishUrgent(const EventPtr &event, const TickType_t ticksToWait) const {
    if (!event) {
        return;
    }

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        Serial.println("[EventBus] Urgently publishing event ID " + String(static_cast<int>(event->id())));

        for (size_t i = 0; i < subscriberCount; ++i) {
            const auto& sub = subscriptions[i];
            if (sub.eventId == event->id()) {
                auto* heapEventPtr = new EventPtr(event);
                xQueueSendToFront(sub.queue, &heapEventPtr, ticksToWait);
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

