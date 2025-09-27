#ifndef TASK_H
#define TASK_H

// Modified from https://github.com/summivox/esp32-cpp-boilerplate/blob/master/main/common/macros.hpp

#include <atomic>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
class Task {
public:
    virtual ~Task() { kill(); }

    /// \returns FreeRTOS task handle if the task has been started; `nullptr` otherwise.
    TaskHandle_t handle() const { return handle_.load(); }
    /// \returns Task name.
    const char* taskName() const { return name; }

    /// Starts the task using default parameters.
    virtual void start() { spawn(); }
    /// Deletes the task.
    virtual void stop() { kill(); }

protected:
    Task(const char *name, const uint32_t stackDepth, const uint32_t priority)
        : name(name), stackDepth(stackDepth), priority(priority) {}

    /// Entry point for the task (must be overridden by the subclass and never return)
    [[noreturn]] virtual void run() = 0;

    /// Starts the task without specifying CPU core affinity.
    /// If the task has already been started, it will be killed then restarted.
    esp_err_t spawn() {
        kill();
        TaskHandle_t handle = nullptr;
        if (xTaskCreate(runAdapter, name, stackDepth, this, priority, &handle) != pdTRUE) {
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
        if (xTaskCreatePinnedToCore(runAdapter, name, stackDepth, this, priority, &handle, cpu) !=
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
    const char* name;
    uint32_t stackDepth;
    uint32_t priority;

    std::atomic<TaskHandle_t> handle_{nullptr};

    static void runAdapter(void *self) { static_cast<Task *>(self)->run(); }

    Task(Task const &) = delete;
    Task(Task &&) = delete;
    void operator=(Task const &t) = delete;
};

#endif //TASK_H
