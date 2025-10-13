#ifndef TASK_H
#define TASK_H

// Modified from https://github.com/summivox/esp32-cpp-boilerplate/blob/master/main/common/macros.hpp

#include <atomic>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "utils/Macros.h"

/// Bare-minimum wrapper base class for a FreeRTOS task. This allows encapsulation of a task
/// alongside with its context.
///
/// \example
/// \code{.cpp}
/// class MyTask : public Task {
///  public:
///   virtual ~MyTask() = default;
///   void start() { Task::spawnSame(/* ... */); }
///   void stop() { Task::kill(); }
///
///  protected:
///   void run() override {
///     // the task entry point is here
///     while (true) {
///       counter_++;
///       vTaskDelay(1);
///     }
///   }
///
///  private:
///   int counter_ = 0;
///   /* ... */
/// };
/// \endcode
///
namespace Priority
{
    constexpr UBaseType_t Minimum = 1;
    constexpr UBaseType_t Background = 2;
    constexpr UBaseType_t Normal = 3;
    constexpr UBaseType_t Realtime = 10;
};

template<uint32_t stackDepth, UBaseType_t priority>
class Task {
public:
    virtual ~Task() { kill(); }

    /// \returns FreeRTOS task handle if the task has been started; `nullptr` otherwise.
    TaskHandle_t handle() const { return handle_.load(); }
    /// \returns Task name.
    virtual const char* taskName() = 0;

    /// Starts the task using default parameters.
    virtual void start() { spawn(); }
    /// Deletes the task.
    virtual void stop() { kill(); }

protected:
    Task() = default;

    /// Entry point for the task (must be overridden by the subclass and never return)
    [[noreturn]] virtual void run() = 0;

    /// Starts the task without specifying CPU core affinity.
    /// If the task has already been started, it will be killed then restarted.
    esp_err_t spawn() {
        kill();
        TaskHandle_t handle = nullptr;
        if (xTaskCreate(runAdapter, taskName(), stackDepth, this, priority, &handle) != pdTRUE) {
            return ESP_ERR_NO_MEM;
        }
        handle_.store(handle);
        return ESP_OK;
    }

    /// Starts the task on the specified CPU core.
    /// If the task has already been started, it will be killed then restarted.
    /// \param cpu Index of the core (e.g. `PRO_CPU_NUM`, `APP_CPU_NUM`).
    esp_err_t spawnPinned(const BaseType_t cpu) {
        kill();
        TaskHandle_t handle = nullptr;
        if (xTaskCreatePinnedToCore(runAdapter, taskName(), stackDepth, this, priority, &handle, cpu) !=
            pdTRUE) {
            return ESP_ERR_NO_MEM;
        }
        handle_.store(handle);
        return ESP_OK;
    }

    /// Starts the task on the same CPU as the caller.
    /// If the task has already been started, it will be killed then restarted.
    esp_err_t spawnSame() {
        return spawnPinned(xPortGetCoreID());
    }

    /// Stops the task if not already stopped.
    void kill() {
        const TaskHandle_t handle = handle_.exchange(nullptr);
        if (handle) {
            vTaskDelete(handle);
        }
    }

private:
    std::atomic<TaskHandle_t> handle_{nullptr};

    static void runAdapter(void *self) {
        auto* task = static_cast<Task *>(self);
        LOG_DEBUG("Task '%s' started on core %d", task->taskName(), xPortGetCoreID());
        task->run();
    }

    Task(Task const &) = delete;
    Task(Task &&) = delete;
    void operator=(Task const &t) = delete;
};

#endif //TASK_H
