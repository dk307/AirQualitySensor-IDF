

#include "hardware/sensors/sps30_sensor_device.h"
#include "hardware/pins.h"
#include "logging/logging_tags.h"
#include "sps30/sps30.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <esp_log.h>
#include <i2cdev.h>

void sps30_sensor_device::init()
{
    sps30_sensor.addr = SPS30_I2C_ADDRESS;
    sps30_sensor.port = I2C_NUM_1;
    sps30_sensor.cfg.mode = I2C_MODE_MASTER;
    sps30_sensor.cfg.sda_io_num = SDAWire;
    sps30_sensor.cfg.scl_io_num = SCLWire;
    sps30_sensor.cfg.master.clk_speed = 100 * 1000; // 100 Kbits
    ESP_ERROR_CHECK(i2c_dev_create_mutex(&sps30_sensor));

    const auto sps_error = sps30_probe();
    if (sps_error == NO_ERROR)
    {
        ESP_LOGI(SENSOR_SPS30_TAG, "SPS30 Found");

        const auto sps_error1 = sps30_start_measurement();
        if (sps_error1 != NO_ERROR)
        {
            ESP_LOGE(SENSOR_SPS30_TAG, "SPS30 start measurement failed with :%d", sps_error1);
            CHECK_THROW_ESP2(ESP_FAIL, "sps30 init failed");
        }
    }
    else
    {
        ESP_LOGE(SENSOR_SPS30_TAG, "SPS30 Probe failed with :%d", sps_error);
        CHECK_THROW_ESP2(ESP_FAIL, "sps30 init failed");
    }
}

std::array<std::tuple<sensor_id_index, float>, 5> sps30_sensor_device::read()
{
    uint16_t ready = 0;
    auto error = sps30_read_data_ready(&ready);

    sps30_measurement measurement{NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN, NAN};
    if ((error == NO_ERROR) && ready)
    {
        error = sps30_read_measurement(&measurement);
        if (error == NO_ERROR)
        {
            ESP_LOGI(SENSOR_SPS30_TAG, "Read SPS30 sensor values PM2.5:%g, PM1:%g, PM4:%g, PM10:%g, Particle Size:%g", measurement.mc_2p5,
                     measurement.mc_1p0, measurement.mc_4p0, measurement.mc_10p0, measurement.typical_particle_size);
        }
        else
        {
            ESP_LOGE(SENSOR_SPS30_TAG, "Failed to read from SPS30 sensor with failed to read measurement error:0x%x", error);
        }
    }
    else
    {
        ESP_LOGE(SENSOR_SPS30_TAG, "Failed to read from SPS30 sensor with data not ready error:0x%x", error);
    }

    return {std::tuple<sensor_id_index, float>{sensor_id_index::pm_10, esp32::round_with_precision(measurement.mc_10p0, 1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::pm_1, esp32::round_with_precision(measurement.mc_1p0, 1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::pm_2_5, esp32::round_with_precision(measurement.mc_2p5, 1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::pm_4, esp32::round_with_precision(measurement.mc_4p0, 1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::typical_particle_size,
                                               esp32::round_with_precision(measurement.typical_particle_size, 0.1)}};
}

std::string sps30_sensor_device::get_error_register_status()
{
    uint32_t device_status_flags{};
    const auto error = sps30_read_device_status_register(&device_status_flags);

    if (error != NO_ERROR)
    {
        return esp32::string::sprintf("Failed to read status with %s", esp_err_to_name(error));
    }

    std::string status;
    status.reserve(128);

    if (device_status_flags & SPS30_DEVICE_STATUS_FAN_ERROR_MASK)
    {
        status += "Fan Error.";
    }
    if (device_status_flags & SPS30_DEVICE_STATUS_LASER_ERROR_MASK)
    {
        status += "Laser Error.";
    }
    if (device_status_flags & SPS30_DEVICE_STATUS_FAN_SPEED_WARNING)
    {
        status += "Fan Speed Warning.";
    }

    if (status.empty())
    {
        status = "Normal";
    }
    return status;
}

bool sps30_sensor_device::clean()
{
    const auto sps_error = sps30_start_manual_fan_cleaning();
    if (sps_error != NO_ERROR)
    {
        ESP_LOGE(SENSOR_SPS30_TAG, "SPS30 manual clean up failed with :%d", sps_error);
        return false;
    }
    else
    {
        ESP_LOGI(SENSOR_SPS30_TAG, "SPS30 manual cleanup started");
        return true;
    }
    return true;
}

esp_err_t sps30_sensor_device::sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count)
{
    sps30_sensor.addr = address;
    I2C_DEV_TAKE_MUTEX(&sps30_sensor);
    I2C_DEV_CHECK(&sps30_sensor, i2c_dev_read(&sps30_sensor, NULL, 0, data, count));
    I2C_DEV_GIVE_MUTEX(&sps30_sensor);
    return ESP_OK;
}

esp_err_t sps30_sensor_device::sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count)
{
    sps30_sensor.addr = address;
    I2C_DEV_TAKE_MUTEX(&sps30_sensor);
    I2C_DEV_CHECK(&sps30_sensor, i2c_dev_write(&sps30_sensor, NULL, 0, data, count));
    I2C_DEV_GIVE_MUTEX(&sps30_sensor);
    return ESP_OK;
}

// glue functions

extern "C" int8_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count)
{
    const auto error = sps30_sensor_device::get_instance().sensirion_i2c_read(address, data, count);
    if (error != ESP_OK)
    {
        ESP_LOGW(SENSOR_SPS30_TAG, "I2C Read Operation Failed with:%s", esp_err_to_name(error));
        return 1;
    }
    return 0;
}

extern "C" int8_t sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count)
{
    const auto error = sps30_sensor_device::get_instance().sensirion_i2c_write(address, data, count);
    if (error != ESP_OK)
    {
        ESP_LOGW(SENSOR_SPS30_TAG, "I2C Write Operation Failed with:%s", esp_err_to_name(error));
        return 1;
    }
    return 0;
}

extern "C" void sensirion_sleep_usec(uint32_t useconds)
{
    vTaskDelay(pdMS_TO_TICKS(useconds / 1000));
}