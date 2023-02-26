#pragma once

#include <esp_timer.h>

namespace esp32
{
__attribute__((unused)) static inline unsigned long millis(void)
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

} // namespace esp32