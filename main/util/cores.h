#pragma once

#include <freertos/FreeRTOS.h>

namespace esp32
{

#ifdef CONFIG_ESP_MAIN_TASK_AFFINITY_CPU0
constexpr BaseType_t main_task_core = 0;
constexpr BaseType_t other_task_core = 1;
#elif defined CONFIG_ESP_MAIN_TASK_AFFINITY_CPU1
constexpr BaseType_t main_task_core = 1;
constexpr BaseType_t other_task_core = 0;
#else
#error "Main Core not defined"
#endif

#ifdef CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_0
constexpr BaseType_t wifi_core = 0;
#elif defined CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1
constexpr BaseType_t wifi_core = 1;
#error "Wifi Core not defined"
#endif

constexpr BaseType_t http_server_core = wifi_core;
constexpr BaseType_t hardware_core = other_task_core;
constexpr BaseType_t display_core = other_task_core;

} // namespace esp32
