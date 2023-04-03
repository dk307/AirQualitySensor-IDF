#pragma once

#include "hardware/sensors/bh1750_sensor_device.h"
#include "hardware/sensors/sensor.h"
#include "hardware/sensors/sht3x_sensor_device.h"
#include "hardware/sensors/sps30_sensor_device.h"
#include "ui/ui_interface.h"
#include "util/psram_allocator.h"
#include "util/singleton.h"
#include <i2cdev.h>

class display;
class config;

class hardware final : public esp32::singleton<hardware>
{
  public:
    void begin();

    const sensor_value &get_sensor(sensor_id_index index) const
    {
        return sensors[static_cast<uint8_t>(index)];
    }

    std::optional<int16_t> get_sensor_value(sensor_id_index index) const;
    sensor_history::sensor_history_snapshot get_sensor_detail_info(sensor_id_index index);

    const sensor_history &get_sensor_history(sensor_id_index index) const
    {
        return (*sensors_history)[static_cast<uint8_t>(index)];
    }

    std::string get_sps30_error_register_status();
    bool clean_sps_30();

  private:
    hardware(config &config, display &display) : config_(config), display_(display), sensor_refresh_task([this] { sensor_task_ftn(); })
    {
    }

    friend class esp32::singleton<hardware>;

    config &config_;
    display &display_;

    // same index as sensor_id_index
    std::array<sensor_value, total_sensors> sensors;
    std::unique_ptr<std::array<sensor_history, total_sensors>, esp32::psram::deleter> sensors_history =
        esp32::psram::make_unique<std::array<sensor_history, total_sensors>>();

    esp32::task sensor_refresh_task;

    using light_sensor_values_t = sensor_history_t<6>;
    light_sensor_values_t light_sensor_values;

#ifdef CONFIG_SHT3X_SENSOR_ENABLE
    // // SHT31
    sht3x_sensor_device &sht3x_sensor{sht3x_sensor_device::create_instance()};
    uint64_t sht3x_sensor_last_read = 0;
#endif

    // // SPS 30
    sps30_sensor_device &sps30_sensor{sps30_sensor_device::create_instance()};
    uint64_t sps30_sensor_last_read = 0;

    // BH1750
    bh1750_sensor_device &bh1750_sensor{bh1750_sensor_device::create_instance()};
    uint64_t bh1750_sensor_last_read = 0;

    void set_sensor_value(sensor_id_index index, float value);

    void read_bh1750_sensor();
#ifdef CONFIG_SHT3X_SENSOR_ENABLE    
    void read_sht3x_sensor();
#endif    
    esp_err_t sps30_i2c_init();
    void read_sps30_sensor();
    uint8_t lux_to_intensity(uint16_t lux);
    void set_auto_display_brightness();

    void sensor_task_ftn();

    template <class T> void read_sensor_if_time(T &sensor, uint64_t &last_read);
};
