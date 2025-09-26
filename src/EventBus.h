#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "Events.h"
#include "utils/Macros.h"

#include <array>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <memory>
#include <vector>

/**
 * @brief Defines the maximum number of event subscriptions allowed in the system.
 * @details This is used to pre-allocate memory and enforce a hard limit to prevent deadlocks.
 */
#define EVENTBUS_MAX_SUBSCRIBERS 64

// The type of item that will be sent on the queues.
// Using std::shared_ptr to automatically manage event lifetime.
using EventPtr = std::shared_ptr<IEvent>;

class EventBus {
    SINGLETON(EventBus)
public:
    /**
     * @brief Subscribes a task's queue to a specific event type.
     * @details This method is thread-safe. It acquires a mutex and will block if the mutex is
     * unavailable, causing the task to yield. The critical section is very short.
     * The subscription will fail if the total number of subscribers has reached EVENTBUS_MAX_SUBSCRIBERS.
     * @param eventId The ID of the event to subscribe to.
     * @param queue The queue handle of the subscribing task. The queue must be created to hold `EventPtr*` items.
     */
    void subscribe(EventId eventId, QueueHandle_t queue);

    /**
     * @brief Unsubscribes a task's queue from a specific event type.
     * @details This method is thread-safe. It acquires a mutex and will block if the mutex is
     * unavailable, causing the task to yield.
     * @param eventId The ID of the event to unsubscribe from.
     * @param queue The queue handle of the subscribing task.
     */
    void unsubscribe(EventId eventId, QueueHandle_t queue);

    /**
     * @brief Publishes an event to all subscribers.
     * @details This method is thread-safe. It acquires a mutex and will block if the mutex is
     * unavailable, causing the task to yield.
     *
     * @param event A shared_ptr to the event to be published.
     * @param ticksToWait The time in ticks to wait for space to become available on a subscriber's
     * queue. Defaults to 0 (do not block). If a subscriber's queue is full and this is 0, the
     * event will be DROPPED for that subscriber.
     */
    void publish(const EventPtr &event, TickType_t ticksToWait = 0) const;

    /**
     * @brief A convenience template to create and publish an event in one call.
     * @details This has the same blocking and thread-safety characteristics as the primary publish() method.
     * NOTE: This convenience overload always uses the default timeout of portMAX_DELAY for publishing.
     * To use a specific timeout, create the EventPtr manually and call the primary publish() method.
     * @tparam T The concrete event class (e.g., API_KeepAliveEvent).
     * @tparam Args The types of the arguments for the event's constructor.
     * @param args The arguments for the event's constructor.
     */
    template <typename T, typename... Args>
    void publish(Args&&... args)
    {
        publish(std::make_shared<T>(std::forward<Args>(args)...), portMAX_DELAY);
    }

    /**
     * @brief Checks for an event on a specific queue, optionally blocking.
     * @param queue The handle to the queue to check.
     * @param ticksToWait The time in ticks to wait for an event to become available. Defaults to 0 (do not block).
     * @return An EventPtr containing the event if one was received, otherwise a nullptr.
     */
    static EventPtr tryReceive(QueueHandle_t queue, TickType_t ticksToWait = 0);

    /**
     * @brief Publishes a high-priority event to all subscribers.
     * @details This places the event at the FRONT of the subscriber's queue.
     * Other behavior is identical to publish().
     * @param event A shared_ptr to the event to be published.
     * @param ticksToWait The time in ticks to wait for space to become available.
     */
    void publishUrgent(const EventPtr &event, TickType_t ticksToWait = portMAX_DELAY) const;

    /**
     * @brief A convenience template to create and publish a high-priority event.
     * @details Places the event at the FRONT of the subscriber's queue.
     * NOTE: This convenience overload always uses the default timeout of portMAX_DELAY.
     * @tparam T The concrete event class.
     * @tparam Args The types of the arguments for the event's constructor.
     * @param args The arguments for the event's constructor.
     */
    template <typename T, typename... Args>
    void publishUrgent(Args&&... args)
    {
        publishUrgent(std::make_shared<T>(std::forward<Args>(args)...), portMAX_DELAY);
    }

private:
    ~EventBus();

    struct Subscription {
        EventId eventId;
        QueueHandle_t queue;
    };

    std::array<Subscription, EVENTBUS_MAX_SUBSCRIBERS> subscriptions;
    size_t subscriber_count = 0;
    SemaphoreHandle_t mutex;
};

#endif // EVENTBUS_H
