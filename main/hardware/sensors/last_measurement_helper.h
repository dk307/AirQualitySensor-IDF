#pragma once

#include "util/misc.h"
#include <optional>
#include <type_traits>

template <class T, unsigned long max_interval_ms>
    requires std::is_copy_assignable_v<T>
class last_measurement_helper
{
  public:
    constexpr static unsigned long max_value_interval_ms = max_interval_ms;

    void store(const T &value)
    {
        last_measurement_value_ = value;
        last_measurement_value_taken_ = esp32::millis();
    }

    bool update(T &value)
    {
        if ((esp32::millis() - last_measurement_value_taken_) <= max_interval_ms)
        {
            if (last_measurement_value_.has_value())
            {
                value = last_measurement_value_.value();
                return true;
            }
        }
        return false;
    }

  private:
    std::optional<T> last_measurement_value_;
    uint64_t last_measurement_value_taken_;
};
