#pragma once

#include "sdkconfig.h"
#include <cstddef>
#include <stdint.h>

enum class sensor_id_index : uint8_t
{
    pm_2_5 = 0,
    first = pm_2_5,
    temperatureF,
    temperatureC,
    humidity,
#if defined CONFIG_SCD30_SENSOR_ENABLE || defined CONFIG_SCD4x_SENSOR_ENABLE
    CO2,
#endif
    pm_1,
    pm_4,
    pm_10,
    typical_particle_size,
    light_intensity,
    last = light_intensity,
};

constexpr auto total_sensors = static_cast<size_t>(sensor_id_index::last) + 1;

enum class sensor_level : uint8_t
{
    no_level = 0,
    level_1 = 1,
    level_2 = 2,
    level_3 = 3,
    level_4 = 4,
    level_5 = 5,
    level_6 = 6,
};
