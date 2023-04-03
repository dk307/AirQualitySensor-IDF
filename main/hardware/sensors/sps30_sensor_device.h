#pragma once

#include "hardware/sensors/sensor_id.h"
#include "hardware/sensors/sps30/sps30.h"
#include "util/singleton.h"
#include <array>
#include <i2cdev.h>
#include <string>
#include <tuple>

class sps30_sensor_device final : public esp32::singleton<sps30_sensor_device>
{
  public:
    void init();
    std::array<std::tuple<sensor_id_index, float>, 5> read();

    std::string get_error_register_status();
    bool clean();

  private:
    i2c_dev_t sps30_sensor{};

    esp_err_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count);
    esp_err_t sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count);

    friend int8_t sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count);
    friend int8_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count);

    sps30_sensor_device() = default;
    friend class esp32::singleton<sps30_sensor_device>;
};