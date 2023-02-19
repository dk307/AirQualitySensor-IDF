#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <bits/functexcept.h>
#include <errno.h>

namespace esp32
{
    class semaphore final
    {
    public:
        semaphore() : handle_(xSemaphoreCreateMutexStatic(&buffer_))
        {
        }

        ~semaphore()
        {
            vSemaphoreDelete(handle_);
        }

        explicit semaphore(const semaphore &) = delete;
        explicit semaphore(const semaphore &&) = delete;
        semaphore &operator=(const semaphore &) = delete;

        void lock()
        {
            const auto result = xSemaphoreTake(handle_, portMAX_DELAY);
            if (result != pdTRUE)
            {
                std::__throw_system_error(EBUSY);
            }
        }

        bool try_lock() noexcept
        {
            return xSemaphoreTake(handle_, 0) == pdTRUE;
        }

        void unlock()
        {
            const auto result = xSemaphoreGive(handle_);
            if (result != pdTRUE)
            {
                std::__throw_system_error(EBUSY);
            }
        }

    private:
        SemaphoreHandle_t handle_;
        StaticSemaphore_t buffer_;
    };
}
