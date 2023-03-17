#pragma once

#include "sensor/sensor.h"
#include "sensor/sensor_id.h"
#include "util/psram_allocator.h"
#include "wifi/wifi_manager.h"

#include <string>
#include <vector>

class ui_interface
{
  public:
    enum class information_type
    {
        network,
        system,
        config
    };

    typedef std::vector<std::pair<std::string, std::string>, esp32::psram::allocator<std::pair<std::string, std::string>>> information_table_type;
    virtual information_table_type get_information_table(information_type type) = 0;

    virtual void set_screen_brightness(uint8_t value) = 0;
    virtual std::optional<int16_t> get_sensor_value(sensor_id_index index) const = 0;
    virtual sensor_history::sensor_history_snapshot get_sensor_detail_info(sensor_id_index index) = 0;
    virtual wifi_status get_wifi_status() = 0;

    virtual void start_wifi_enrollment() = 0;
    virtual void stop_wifi_enrollment() = 0;

    virtual bool clean_sps_30() = 0;
};