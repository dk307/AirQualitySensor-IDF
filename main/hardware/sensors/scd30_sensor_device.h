#pragma once
#include "sdkconfig.h"

#ifdef CONFIG_SCD30_SENSOR_ENABLE
#include "hardware/sensors/last_measurement_helper.h"
#include "hardware/sensors/sensor_id.h"
#include "util/singleton.h"
#include <array>
#include <i2cdev.h>
#include <scd30.h>
#include <tuple>

class scd30_sensor_device final : public esp32::singleton<scd30_sensor_device>
{
  public:
    void init();
    std::array<std::tuple<sensor_id_index, float>, 4> read();

    uint8_t get_initial_delay();

  private:
    i2c_dev_t scd30_sensor_{};
    last_measurement_helper<std::tuple<float, float, float, float>, 20000> last_measurement_value_;

    scd30_sensor_device() = default;
    friend class esp32::singleton<scd30_sensor_device>;
};
#endif