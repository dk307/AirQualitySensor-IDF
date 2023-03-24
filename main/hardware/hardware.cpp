#include "hardware/hardware.h"
#include "app_events.h"
#include "config/config_manager.h"
#include "hardware/display.h"
#include "hardware/sd_card.h"
#include "homekit/homekit_integration.h"
#include "logging/logging_tags.h"
#include "util/cores.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include "util/misc.h"
#include "wifi/wifi_manager.h"
#include "wifi/wifi_sta.h"
#include <driver/i2c.h>
#include <esp_chip_info.h>
#include <esp_efuse.h>
#include <esp_flash.h>
#include <esp_mac.h>
#include <esp_netif_types.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <memory>

hardware hardware::instance;

void hardware::set_screen_brightness(uint8_t value)
{
    if (current_brightness_ != value)
    {
        ESP_LOGI(DISPLAY_TAG, "Setting display brightness to %d", value);
        display_instance_.set_brightness(std::max<uint8_t>(30, value));
        current_brightness_ = value;
    }
}

std::optional<int16_t> hardware::get_sensor_value(sensor_id_index index) const
{
    auto &&sensor = get_sensor(index);
    return sensor.get_value_as<int16_t>();
}

sensor_history::sensor_history_snapshot hardware::get_sensor_detail_info(sensor_id_index index)
{
    return (*sensors_history)[static_cast<size_t>(index)].get_snapshot(sensor_history::reads_per_minute);
}

wifi_status hardware::get_wifi_status()
{
    return wifi_manager::instance.get_wifi_status();
}

bool hardware::clean_sps_30()
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

void hardware::start_wifi_enrollment()
{
    wifi_manager::instance.start_wifi_enrollment();
}

void hardware::stop_wifi_enrollment()
{
    wifi_manager::instance.stop_wifi_enrollment();
}

void hardware::forget_homekit_pairings()
{
    homekit_integration::instance.forget_pairings();
}
void hardware::reenable_homekit_pairing()
{
    homekit_integration::instance.reenable_pairing();
}

void hardware::begin()
{
    display_instance_.start();
    current_brightness_ = display_instance_.get_brightness();
    CHECK_THROW_ESP(i2cdev_init());
    sensor_refresh_task.spawn_pinned("sensor_task", 4 * 1024, esp32::task::default_priority, esp32::hardware_core);
}

void hardware::set_sensor_value(sensor_id_index index, float value, float precision)
{
    bool changed;
    const auto i = static_cast<size_t>(index);
    if (!std::isnan(value))
    {
        (*sensors_history)[i].add_value(value);
        changed = sensors[i].set_value(value, precision);
        ESP_LOGI(HARDWARE_TAG, "Updated for sensor:%.*s Value:%g", get_sensor_name(index).size(), get_sensor_name(index).data(),
                 sensors[i].get_value().value_or(NAN));
    }
    else
    {
        ESP_LOGW(HARDWARE_TAG, "Got an invalid value for sensor:%.*s", get_sensor_name(index).size(), get_sensor_name(index).data());
        (*sensors_history)[i].clear();
        changed = sensors[i].set_invalid_value();
    }

    if (changed)
    {
        CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, SENSOR_VALUE_CHANGE, index));
    }
}

void hardware::sensor_task_ftn()
{
    try
    {
        ESP_LOGI(HARDWARE_TAG, "Sensor task started on core:%d", xPortGetCoreID());

        // sps30
        CHECK_THROW_ESP(sps30_i2c_init());

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

        // bh1750
        CHECK_THROW_ESP(bh1750_init_desc(&bh1750_sensor, BH1750_ADDR_LO, I2C_NUM_1, SDAWire, SCLWire));
        CHECK_THROW_ESP(bh1750_setup(&bh1750_sensor, BH1750_MODE_CONTINUOUS, BH1750_RES_HIGH));

        const auto bh1750_wait = pdMS_TO_TICKS(180);

        // sht3x
        CHECK_THROW_ESP(sht3x_init_desc(&sht3x_sensor, SHT3X_I2C_ADDR_GND, I2C_NUM_1, SDAWire, SCLWire));
        CHECK_THROW_ESP(sht3x_init(&sht3x_sensor));

        const auto sht3x_wait = sht3x_get_measurement_duration(SHT3X_HIGH);

        // Wait until all sensors are ready
        vTaskDelay(std::max<uint64_t>(bh1750_wait, sht3x_wait));

        do
        {
            read_bh1750_sensor();
            set_auto_display_brightness();
            read_sht3x_sensor();
            read_sps30_sensor();

            vTaskDelay(pdMS_TO_TICKS(500));
        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "Hardware Task Failure:%s", ex.what());
        throw;
    }

    vTaskDelete(NULL);
}

void hardware::read_bh1750_sensor()
{
    std::optional<uint16_t> lux;
    uint16_t level_lux = 0;
    auto err = bh1750_read(&bh1750_sensor, &level_lux);
    if (err != ESP_OK)
    {
        ESP_LOGW(SENSOR_BH1750_TAG, "Failed to read sensor with %s", esp_err_to_name(err));
    }
    else
    {
        lux = level_lux;
        light_sensor_values.add_value(level_lux);
    }

    const auto now = esp32::millis();
    if (now - bh1750_sensor_last_read >= sensor_history::sensor_interval)
    {
        if (lux.has_value())
        {
            const auto value = lux.value();
            ESP_LOGI(SENSOR_BH1750_TAG, "Read BH1750 value:%d lux", value);
            bh1750_sensor_last_read = now;
        }
        else
        {
            ESP_LOGW(SENSOR_BH1750_TAG, "Failed to read from BH1750");
        }

        set_sensor_value(sensor_id_index::light_intensity, lux.has_value() ? lux.value() : NAN, 1);
    }
}

void hardware::read_sht3x_sensor()
{
    const auto now = esp32::millis();
    if (now - sht3x_sensor_last_read >= sensor_history::sensor_interval)
    {
        float temperatureC = NAN;
        float humidity = NAN;
        auto err = sht3x_measure(&sht3x_sensor, &temperatureC, &humidity);
        float temperatureF = NAN;
        if (err == ESP_OK)
        {
            temperatureF = (temperatureC * 1.8) + 32;
            ESP_LOGI(SENSOR_SHT31_TAG, "Read SHT31 sensor values:%g F, %g C  %g %%", temperatureF, temperatureC, humidity);
            sht3x_sensor_last_read = now;
        }
        else
        {
            ESP_LOGE(SENSOR_SHT31_TAG, "Failed to read from SHT3x sensor with error:%s", esp_err_to_name(err));
        }

        set_sensor_value(sensor_id_index::temperatureC, temperatureC, 0.01);
        set_sensor_value(sensor_id_index::temperatureF, temperatureF, 0.1);
        set_sensor_value(sensor_id_index::humidity, humidity, 1);
    }
}

void hardware::read_sps30_sensor()
{
    const auto now = esp32::millis();
    if (now - sps30_sensor_last_read >= sensor_history::sensor_interval)
    {
        uint16_t ready = 0;

        auto error = sps30_read_data_ready(&ready);

        auto val_10 = NAN;
        auto val_1 = NAN;
        auto val_2_5 = NAN;
        auto val_4 = NAN;
        auto val_p = NAN;

        if ((error == NO_ERROR) && ready)
        {
            sps30_measurement m{};
            error = sps30_read_measurement(&m);
            if (error == NO_ERROR)
            {
                sps30_sensor_last_read = now;

                val_10 = (m.mc_10p0);
                val_1 = (m.mc_1p0);
                val_2_5 = (m.mc_2p5);
                val_4 = (m.mc_4p0);
                val_p = (m.typical_particle_size);

                ESP_LOGI(SENSOR_SPS30_TAG, "Read SPS30 sensor values PM2.5:%g, PM1:%g, PM4:%g, PM10:%g, Particle Size:%g", val_2_5, val_1, val_4,
                         val_10, val_p);
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

        set_sensor_value(sensor_id_index::pm_10, val_10, 1);
        set_sensor_value(sensor_id_index::pm_1, val_1, 1);
        set_sensor_value(sensor_id_index::pm_2_5, val_2_5, 1);
        set_sensor_value(sensor_id_index::pm_4, val_4, 1);
        set_sensor_value(sensor_id_index::typical_particle_size, val_p, 0.1);
    }
}

uint8_t hardware::lux_to_intensity(uint16_t lux)
{
    if (lux != 0)
    {
        // https://learn.microsoft.com/en-us/windows/win32/sensorsapi/understanding-and-interpreting-lux-values
        const auto intensity = (std::log10(lux) / 5) * 255;
        return intensity;
    }
    else
    {
        return 0;
    }
}

void hardware::set_auto_display_brightness()
{
    const auto config_brightness = config::instance.get_manual_screen_brightness();

    uint8_t required_brightness;
    if (config_brightness.has_value())
    {
        required_brightness = config_brightness.value();
        ESP_LOGD(SENSOR_BH1750_TAG, "Configured brightness:%d", required_brightness);
    }
    else
    {
        const auto avg_lux = light_sensor_values.get_average();
        ESP_LOGD(SENSOR_BH1750_TAG, "Average lux:%d", avg_lux.value_or(128));
        required_brightness = lux_to_intensity(avg_lux.value_or(128));
    }

    ESP_LOGD(SENSOR_BH1750_TAG, "Required brightness:%d", required_brightness);
    set_screen_brightness(required_brightness);
}

esp_err_t hardware::sps30_i2c_init()
{
    sps30_sensor.addr = SPS30_I2C_ADDRESS;
    sps30_sensor.port = I2C_NUM_1;
    sps30_sensor.cfg.mode = I2C_MODE_MASTER;
    sps30_sensor.cfg.sda_io_num = SDAWire;
    sps30_sensor.cfg.scl_io_num = SCLWire;
    sps30_sensor.cfg.master.clk_speed = 100 * 1000; // 100 Kbits
    ESP_ERROR_CHECK(i2c_dev_create_mutex(&sps30_sensor));

    return ESP_OK;
}

esp_err_t hardware::sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count)
{
    sps30_sensor.addr = address;
    I2C_DEV_TAKE_MUTEX(&sps30_sensor);
    I2C_DEV_CHECK(&sps30_sensor, i2c_dev_read(&sps30_sensor, NULL, 0, data, count));
    I2C_DEV_GIVE_MUTEX(&sps30_sensor);
    return ESP_OK;
}

esp_err_t hardware::sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count)
{
    sps30_sensor.addr = address;
    I2C_DEV_TAKE_MUTEX(&sps30_sensor);
    I2C_DEV_CHECK(&sps30_sensor, i2c_dev_write(&sps30_sensor, NULL, 0, data, count));
    I2C_DEV_GIVE_MUTEX(&sps30_sensor);
    return ESP_OK;
}
