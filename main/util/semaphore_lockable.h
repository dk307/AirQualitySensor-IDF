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
        semaphore() : handle(xSemaphoreCreateMutexStatic(&buffer))
        {
        }

        ~semaphore()
        {
            vSemaphoreDelete(handle);
        }

        explicit semaphore(const semaphore &other) = delete;
        semaphore &operator=(const semaphore &other) = delete;

        void lock()
        {
            const auto result = xSemaphoreTake(handle, portMAX_DELAY);
            if (result != pdTRUE)
            {
                std::__throw_system_error(EBUSY);
            }
        }

        bool try_lock() noexcept
        {
            return xSemaphoreTake(handle, 0) == pdTRUE;
        }

        void unlock()
        {
            const auto result = xSemaphoreGive(handle);
            if (result != pdTRUE)
            {
                std::__throw_system_error(EBUSY);
            }
        }

    private:
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    };
}
