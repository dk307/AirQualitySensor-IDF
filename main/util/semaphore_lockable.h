#pragma once

#include "util/noncopyable.h"
#include <bits/functexcept.h>
#include <errno.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace esp32
{
class semaphore final : esp32::noncopyable
{
  public:
    semaphore() : handle_(xSemaphoreCreateMutexStatic(&buffer_))
    {
    }

    ~semaphore()
    {
        vSemaphoreDelete(handle_);
    }

    explicit semaphore(const semaphore &&) = delete;

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
} // namespace esp32
