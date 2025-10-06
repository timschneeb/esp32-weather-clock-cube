#include "EventBus.h"

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
            LOG_ERROR("Max subscribers reached. Subscription failed.");
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
    RETURN_IF_FAILED(event != nullptr, "Event is null");

    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        LOG_DEBUG("Dispatching %s %s", named_enum::name(event->id()), (urgent ? "(urgent)" : ""));
        for (size_t i = 0; i < subscriberCount; ++i) {
            const auto&[eventId, queue] = subscriptions[i];
            if (eventId == event->id()) {
                auto* heapEventPtr = new EventPtr(event);
                BaseType_t result;
                if (urgent)
                    result = xQueueSendToFront(queue, &heapEventPtr, ticksToWait);
                else
                    result = xQueueSend(queue, &heapEventPtr, ticksToWait);

                if (result == errQUEUE_FULL) {
                    LOG_ERROR("Queue full, event dropped for subscriber %u", i);
                    DISP_PANIC(String(
                        "Queue full\n" +
                        String(named_enum::name(event->id())) + " dropped\n"
                        "Subscriber " + String(i) + "\n"
                        "Wait count: " + String(uxQueueMessagesWaiting(queue))
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

