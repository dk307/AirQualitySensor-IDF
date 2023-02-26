#pragma once

#include "display.h"
#include "sensor/sensor.h"
#include "ui/ui_interface.h"
#include "util/psram_allocator.h"

#include "hardware/sps30/sps30.h"

#include <bh1750.h>
#include <i2cdev.h>
#include <sht3x.h>

class hardware final : ui_interface
{
  public:
    void pre_begin();
    void begin();

    static hardware instance;

    const sensor_value &get_sensor(sensor_id_index index) const
    {
        return sensors[static_cast<uint8_t>(index)];
    }

    const sensor_history &get_sensor_history(sensor_id_index index) const
    {
        return (*sensors_history)[static_cast<uint8_t>(index)];
    }

    void set_main_screen()
    {
        display_instance_.set_main_screen();
    }

    // ui_interface
    information_table_type get_information_table(information_type type) override;
    void set_screen_brightness(uint8_t value) override;
    std::optional<sensor_value::value_type> get_sensor_value(sensor_id_index index) const override;
    sensor_history::sensor_history_snapshot get_sensor_detail_info(sensor_id_index index) override;
    wifi_status get_wifi_status() override;
    bool clean_sps_30() override;

    esp_err_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count);
    esp_err_t sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count);

  private:
    hardware() : sensor_refresh_task(std::bind(&hardware::sensor_task_ftn, this))
    {
    }

    display display_instance_{*this};
    uint8_t current_brightness_{0};

    // // same index as sensor_id_index
    std::array<sensor_value, total_sensors> sensors;
    std::unique_ptr<std::array<sensor_history, total_sensors>, esp32::psram::deleter> sensors_history =
        esp32::psram::make_unique<std::array<sensor_history, total_sensors>>();

    esp32::task sensor_refresh_task;

    using light_sensor_values_t = sensor_history_t<sensor_value::value_type, 10>;
    light_sensor_values_t light_sensor_values;

    const gpio_num_t SDAWire = GPIO_NUM_11;
    const gpio_num_t SCLWire = GPIO_NUM_10;

    // // SHT31
    sht3x_t sht3x_sensor{};
    uint64_t sht3x_sensor_last_read = 0;

    // // SPS 30
    i2c_dev_t sps30_sensor{};
    uint64_t sps30_sensor_last_read = 0;

    // BH1750
    i2c_dev_t bh1750_sensor{};
    uint64_t bh1750_sensor_last_read = 0;

    void set_sensor_value(sensor_id_index index, const std::optional<sensor_value::value_type> &value);

    void read_bh1750_sensor();
    void read_sht3x_sensor();
    esp_err_t sps30_i2c_init();
    void read_sps30_sensor();
    uint8_t lux_to_intensity(sensor_value::value_type lux);
    void set_auto_display_brightness();

    static std::optional<sensor_value::value_type> round_value(float val, int places = 0);
    void sensor_task_ftn();

    // info
    static std::string get_default_mac_address();
    static std::string get_version();
    static std::string get_reset_reason_string();
    static std::string get_chip_details();
    static std::string get_heap_info_str(uint32_t caps);
    static std::string get_up_time();

    std::string get_sps30_error_register_status();
};
