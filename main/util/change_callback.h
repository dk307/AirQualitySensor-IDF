#pragma once

#include "semaphore_lockable.h"

#include <vector>
#include <functional>
#include <mutex>

namespace esp32
{
    class change_callback
    {
    public:
        void add_callback(const std::function<void()> &func) const
        {
            std::lock_guard<esp32::semaphore> lock(list_mutex);
            change_callbacks.push_back(func);
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
            std::lock_guard<esp32::semaphore> lock(list_mutex);
            return change_callbacks;
        }

        mutable esp32::semaphore list_mutex;
        mutable std::vector<std::function<void()>> change_callbacks;
    };
}