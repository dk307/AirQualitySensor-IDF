#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <bits/functexcept.h>
#include <errno.h>

namespace esp32
{
    template <class T, std::size_t TSize>
    class static_queue
    {
    public:
        static_assert(std::is_pod<T>::value, "T must be POD");
        constexpr static size_t item_size = sizeof(T);
        constexpr static size_t queue_size = TSize;

        static_queue()
        {
            queue_ = xQueueCreateStatic(TSize,
                                        item_size,
                                        reinterpret_cast<uint8_t *>(queue_data_.data()),
                                        &static_queue_esp_);

            configASSERT(queue_);
        }

        ~static_queue()
        {
            vQueueDelete(queue_);
        }

        bool enqueue(const T &item, TickType_t timeout)
        {
            const auto success = xQueueSendToBack(queue_, &item, timeout);
            return success == pdTRUE ? true : false;
        }

        bool dequeue(T &item, TickType_t timeout)
        {
            auto success = xQueueReceive(queue_, &item, timeout);
            return success == pdTRUE ? true : false;
        }

        bool peek(T &item, TickType_t timeout)
        {
            auto success = xQueuePeek(queue_, &item, timeout);
            return success == pdTRUE ? true : false;
        }

        bool is_empty() const
        {
            const auto cnt = uxQueueMessagesWaiting(queue_);
            return cnt == 0 ? true : false;
        }

    private:
        std::array<T, TSize> queue_data_;
        StaticQueue_t static_queue_esp_;
        QueueHandle_t queue_;
    };

}
