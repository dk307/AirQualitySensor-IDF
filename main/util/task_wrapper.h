#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace esp32
{
    class task
    {
    public:
        constexpr static uint32_t default_priority = 0;

        task(const std::function<void(void)> &call) : call(call)
        {
        }
        ~task() { kill(); }

        TaskHandle_t handle() const { return handle_.load(); }

        /// Starts the task without specifying CPU affinity.
        esp_err_t spawn(const char *name, uint32_t stack_depth, uint32_t priority)
        {
            kill();
            TaskHandle_t handle = nullptr;
            if (xTaskCreate(run_adapter, name, stack_depth, this, priority, &handle) != pdTRUE)
            {
                return ESP_ERR_NO_MEM;
            }
            handle_.store(handle);
            return ESP_OK;
        }

        /// Starts the task on the specified CPU.
        esp_err_t spawn_pinned(const char *name, uint32_t stack_depth, uint32_t priority, BaseType_t cpu)
        {
            kill();
            TaskHandle_t handle = nullptr;
            if (xTaskCreatePinnedToCore(run_adapter, name, stack_depth, this, priority, &handle, cpu) !=
                pdTRUE)
            {
                return ESP_ERR_NO_MEM;
            }
            handle_.store(handle);
            return ESP_OK;
        }

        /// Starts the task on the same CPU as the caller.
        esp_err_t spawn_same(const char *name, uint32_t stack_depth, uint32_t priority)
        {
            return spawn_pinned(name, stack_depth, priority, xPortGetCoreID());
        }

        /// Stops the task if not already stopped.
        void kill()
        {
            const TaskHandle_t handle = handle_.exchange(nullptr);
            if (handle)
            {
                vTaskDelete(handle);
            }
        }

    protected:
        void run()
        {
            call();
        }

    private:
        std::function<void(void)> call;
        std::atomic<TaskHandle_t> handle_{nullptr};

        task(const task &) = delete;
        task(task &&) = delete;
        task &operator=(const task &) = delete;

        static void run_adapter(void *self) { reinterpret_cast<task *>(self)->run(); }
    };
}
