#pragma once

#include "hardware/sensors/sensor_id.h"
#include <hap_apple_chars.h>
#include <hap_apple_servs.h>
#include <string_view>

struct homekit_definition
{
  public:
    constexpr homekit_definition(sensor_id_index sensor, const std::string_view &service_type_uuid, const std::string_view &cha_type_uuid,
                                 const std::string_view &uint_str)
        : sensor_(sensor), service_type_uuid_(service_type_uuid), cha_type_uuid_(cha_type_uuid), uint_str_(uint_str)
    {
    }

    constexpr sensor_id_index get_sensor() const
    {
        return sensor_;
    }

    constexpr const std::string_view &get_service_type_uuid() const
    {
        return service_type_uuid_;
    }

    constexpr const std::string_view &get_cha_type_uuid() const
    {
        return cha_type_uuid_;
    }

    constexpr const std::string_view &get_uint_str() const
    {
        return uint_str_;
    }

  private:
    const sensor_id_index sensor_;
    const std::string_view service_type_uuid_;
    const std::string_view cha_type_uuid_;
    const std::string_view uint_str_;
};

constexpr auto homekit_definitions = std::to_array<homekit_definition>({
    homekit_definition{sensor_id_index::pm_2_5, HAP_SERV_UUID_AIR_QUALITY_SENSOR, HAP_CHAR_UUID_PM_2_5_DENSITY, std::string_view()},
    homekit_definition{sensor_id_index::temperatureC, HAP_SERV_UUID_TEMPERATURE_SENSOR, HAP_CHAR_UUID_CURRENT_TEMPERATURE, HAP_CHAR_UNIT_CELSIUS},
    homekit_definition{sensor_id_index::humidity, HAP_SERV_UUID_HUMIDITY_SENSOR, HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY, HAP_CHAR_UNIT_PERCENTAGE},
    homekit_definition{sensor_id_index::pm_10, HAP_SERV_UUID_AIR_QUALITY_SENSOR, HAP_CHAR_UUID_PM_10_DENSITY, std::string_view()},
    homekit_definition{sensor_id_index::light_intensity, HAP_SERV_UUID_LIGHT_SENSOR, HAP_CHAR_UUID_CURRENT_AMBIENT_LIGHT_LEVEL, HAP_CHAR_UNIT_LUX},
#ifdef CONFIG_SCD30_SENSOR_ENABLE
    homekit_definition{sensor_id_index::CO2, HAP_SERV_UUID_AIR_QUALITY_SENSOR, HAP_CHAR_UUID_CARBON_DIOXIDE_LEVEL, std::string_view()},
#endif
});

constexpr std::string_view primary_service{HAP_SERV_UUID_AIR_QUALITY_SENSOR};
