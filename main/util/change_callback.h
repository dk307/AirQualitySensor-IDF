#pragma once

#include "semaphore_lockable.h"

#include <functional>
#include <mutex>
#include <vector>


namespace esp32
{
class change_callback
{
  public:
    void add_callback(const std::function<void()> &func) const
    {
        std::lock_guard<esp32::semaphore> lock(list_mutex_);
        change_callbacks_.push_back(func);
    }

    void call_change_listeners() const
    {
        for (auto &&ftn : get_copy())
        {
            ftn();
        }
    }

  private:
    std::vector<std::function<void()>> get_copy() const
    {
        std::lock_guard<esp32::semaphore> lock(list_mutex_);
        return change_callbacks_;
    }

    mutable esp32::semaphore list_mutex_;
    mutable std::vector<std::function<void()>> change_callbacks_;
};
} // namespace esp32