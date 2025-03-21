#pragma once
#include "sdkconfig.h"

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
#include "hardware/sensors/last_measurement_helper.h"
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

    bool factory_reset();

  private:
    i2c_dev_t scd4x_sensor_{};
    last_measurement_helper<std::tuple<float, float, float, float>, 60000> last_measurement_value_;

    scd4x_sensor_device() = default;
    friend class esp32::singleton<scd4x_sensor_device>;
};
#endif