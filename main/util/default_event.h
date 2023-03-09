#pragma once

#include "exceptions.h"
#include "noncopyable.h"
#include <esp_event.h>
#include <functional>
#include <span>

namespace esp32
{

// class default_event_subscriber : noncopyable
// {
//   public:
//     using callback_t = std::function<void(esp_event_base_t, int32_t, void *)>;

//     default_event_subscriber(esp_event_base_t event_base, int32_t event_id, const callback_t &callback)
//         : event_base_(event_base), event_id_(event_id), callback_(callback)
//     {
//     }

//     ~default_event_subscriber()
//     {
//         unsubscribe();
//     }

//     void subscribe()
//     {
//         assert(!instance_event_);
//         CHECK_THROW_ESP(esp_event_handler_instance_register(event_base_, event_id_, event_handler, this, &instance_event_));
//     }

//     void unsubscribe()
//     {
//         if (instance_event_)
//         {
//             esp_event_handler_instance_unregister(event_base_, event_id_, instance_event_);
//             instance_event_ = nullptr;
//         }
//     }

//   private:
//     const esp_event_base_t event_base_;
//     const int32_t event_id_;
//     const callback_t callback_;
//     esp_event_handler_instance_t instance_event_{};

//     static void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
//     {
//         auto p_this = reinterpret_cast<default_event_subscriber *>(event_handler_arg);
//         p_this->callback_(event_base, event_id, event_data);
//     }
// };

template <class T> class default_event_subscriber_typed : noncopyable
{
  public:
    using callback_t = std::function<void(esp_event_base_t, int32_t, T t)>;

    default_event_subscriber_typed(esp_event_base_t event_base, int32_t event_id, const callback_t &callback)
        : event_base_(event_base), event_id_(event_id), callback_(callback)
    {
    }

    ~default_event_subscriber_typed()
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
    const callback_t callback_;
    esp_event_handler_instance_t instance_event_{};

    static void event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        auto p_this = reinterpret_cast<default_event_subscriber_typed<T> *>(event_handler_arg);
        p_this->callback_(event_base, event_id, std::forward<T>(*reinterpret_cast<T *>(event_data)));
    }
};

using default_event_subscriber = default_event_subscriber_typed<void *>;

template <class T> inline esp_err_t event_post(esp_event_base_t event_base, int32_t event_id, T t, TickType_t ticks_to_wait = portMAX_DELAY)
{
    using final_T = std::remove_reference<T>::type;
    return esp_event_post(event_base, event_id, &t, sizeof(final_T), ticks_to_wait);
}

inline esp_err_t event_post(esp_event_base_t event_base, int32_t event_id, TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post(event_base, event_id, NULL, 0, ticks_to_wait);
}

} // namespace esp32