#pragma once

#include "hardware/sensors/sensor_id.h"
#include "util/singleton.h"
#include <array>
#include <i2cdev.h>
#include <sht3x.h>
#include <tuple>

class sht3x_sensor_device : public esp32::singleton<sht3x_sensor_device>
{
  public:
    void init();
    std::array<std::tuple<sensor_id_index, float>, 3> read();

  private:
    sht3x_t sht3x_sensor{};

    sht3x_sensor_device() = default;
    friend class esp32::singleton<sht3x_sensor_device>;
};