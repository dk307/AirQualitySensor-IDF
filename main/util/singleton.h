#pragma once
#include "noncopyable.h"
#include <freertos/FreeRTOS.h>
#include <utility>

namespace esp32
{
template <class T> class singleton
{
  public:
    template <class... Args> static T &create_instance(Args &&...args)
    {
        configASSERT(!instance);
        instance = new T{std::forward<Args>(args)...};
        return *instance;
    }

    explicit singleton<T>(const singleton<T> &) = delete;
    explicit singleton<T>(singleton<T> &&) = delete;
    const singleton<T> &operator=(const singleton<T> &) = delete;

  protected:
    singleton() = default;
    ~singleton() = default;

    static T &get_instance()
    {
        return *instance;
    }

    static T *instance;
};

template <class T> T *singleton<T>::instance = nullptr;
} // namespace esp32