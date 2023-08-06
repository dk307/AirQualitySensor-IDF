#pragma once

#include <cmath>
#include <esp_timer.h>

namespace esp32
{
__attribute__((unused)) static inline unsigned long millis(void)
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

__attribute__((unused)) static inline float round_with_precision(float value, float precision)
{
    return (!std::isnan(value)) ? std::round(value / precision) * precision : value;
}

} // namespace esp32