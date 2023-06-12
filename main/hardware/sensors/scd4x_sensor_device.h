#pragma once
#include "sdkconfig.h"

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
#include "hardware/sensors/sensor_id.h"
#include "util/singleton.h"
#include <array>
#include <i2cdev.h>
#include <scd4x.h>
#include <tuple>

class scd4x_sensor_device final : public esp32::singleton<scd4x_sensor_device>
{
  public:
    void init();
    std::array<std::tuple<sensor_id_index, float>, 4> read();

    uint8_t get_initial_delay();

  private:
    i2c_dev_t scd4x_sensor_{};

    const uint16_t sensor_interval_s_;
    const TickType_t max_wait_ticks_;

    scd4x_sensor_device(uint16_t sensor_interval_s, uint32_t max_wait_ms)
        : sensor_interval_s_(sensor_interval_s), max_wait_ticks_(pdMS_TO_TICKS(max_wait_ms))
    {
    }
    bool wait_till_data_ready();
    friend class esp32::singleton<scd4x_sensor_device>;
};
#endif