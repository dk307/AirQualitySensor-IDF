#pragma once

#include "exceptions.h"
#include "noncopyable.h"
#include <esp_event.h>
#include <functional>

namespace esp32
{
class default_event_subscriber : noncopyable
{
  public:
    using callback_t = std::function<void(esp_event_base_t, int32_t, void *)>;

    default_event_subscriber(esp_event_base_t event_base, int32_t event_id, const callback_t &callback)
        : event_base_(event_base), event_id_(event_id), callback_(callback)
    {
    }

    ~default_event_subscriber()
    {
        unsubscribe();
    }

    void subscribe()
    {
        assert(!instance_event_);
        CHECK_THROW_ESP(esp_event_handler_instance_register(event_base_, event_id_, event_handler, this, &instance_event_));
    }

    void unsubscribe()
    {
        if (instance_event_)
        {
            esp_event_handler_instance_unregister(event_base_, event_id_, instance_event_);
            instance_event_ = nullptr;
        }
    }

  private:
    const esp_event_base_t event_base_;
    const int32_t event_id_;
    callback_t callback_;
    esp_event_handler_instance_t instance_event_{};

    static void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        auto p_this = reinterpret_cast<default_event_subscriber *>(event_handler_arg);
        p_this->callback_(event_base, event_id, event_data);
    }
};
} // namespace esp32