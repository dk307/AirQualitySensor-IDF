#pragma once

#include "hardware/sensors/sensor_id.h"
#include "util/singleton.h"
#include <array>
#include <i2cdev.h>
#include <sht3x.h>
#include <tuple>

class bh1750_sensor_device final : public esp32::singleton<bh1750_sensor_device>
{
  public:
    void init();
    std::array<std::tuple<sensor_id_index, float>, 1> read();

    TickType_t get_initial_delay();

  private:
    i2c_dev_t bh1750_sensor_{};

    bh1750_sensor_device() = default;
    friend class esp32::singleton<bh1750_sensor_device>;
};