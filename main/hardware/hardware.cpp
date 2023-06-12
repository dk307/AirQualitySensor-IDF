#include "hardware/hardware.h"
#include "app_events.h"
#include "config/config_manager.h"
#include "hardware/display/display.h"
#include "hardware/sd_card.h"
#include "homekit/homekit_integration.h"
#include "logging/logging_tags.h"
#include "pins.h"
#include "util/cores.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include "util/misc.h"
#include <driver/i2c.h>
#include <esp_log.h>

template <class T> void hardware::read_sensor_if_time(T &sensor, uint64_t &last_read)
{
    const auto now = esp32::millis();
    if (now - last_read >= sensor_history::sensor_interval)
    {
        for (auto &&value : sensor.read())
        {
            set_sensor_value(std::get<0>(value), std::get<1>(value));
        }
        last_read = now;
    }
}

float hardware::get_sensor_value(sensor_id_index index) const
{
    auto &&sensor = get_sensor(index);
    return sensor.get_value();
}

sensor_history::sensor_history_snapshot hardware::get_sensor_detail_info(sensor_id_index index)
{
    return (*sensors_history_)[static_cast<size_t>(index)].get_snapshot(sensor_history::reads_per_minute);
}

bool hardware::clean_sps_30()
{
    return sps30_sensor_.clean();
}

void task()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDAWire;
    conf.scl_io_num = SCLWire;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config(I2C_NUM_1, &conf);

    i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, 0);

    while (1)
    {
        esp_err_t res;
        printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
        printf("00:         ");
        for (uint8_t i = 3; i < 0x78; i++)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
            i2c_master_stop(cmd);
    
            res = i2c_master_cmd_begin(I2C_NUM_1, cmd, 10 / portTICK_PERIOD_MS);
            if (i % 16 == 0)
                printf("\n%.2x:", i);
            if (res == 0)
                printf(" %.2x", i);
            else
                printf(" --");
            i2c_cmd_link_delete(cmd);
        }
        printf("\n\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void hardware::begin()
{
    CHECK_THROW_ESP(i2cdev_init());
    sensor_refresh_task_.spawn_pinned("sensor_task", 4 * 1024, esp32::task::default_priority, esp32::hardware_core);
}




void hardware::set_sensor_value(sensor_id_index index, float value)
{
    bool changed;
    const auto i = static_cast<size_t>(index);
    if (!std::isnan(value))
    {
        (*sensors_history_)[i].add_value(value);
        changed = sensors_[i].set_value(value);
        ESP_LOGI(HARDWARE_TAG, "Updated for sensor:%.*s Value:%g", get_sensor_name(index).size(), get_sensor_name(index).data(),
                 sensors_[i].get_value());
    }
    else
    {
        ESP_LOGW(HARDWARE_TAG, "Got an invalid value for sensor:%.*s", get_sensor_name(index).size(), get_sensor_name(index).data());
        (*sensors_history_)[i].clear();
        changed = sensors_[i].set_invalid_value();
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

        task();

        TickType_t initial_delay = 0;

        sps30_sensor_.init();
        bh1750_sensor_.init();
        initial_delay = std::max(initial_delay, bh1750_sensor_.get_initial_delay());

#ifdef CONFIG_SHT3X_SENSOR_ENABLE
        sht3x_sensor_.init();
        initial_delay = std::max<TickType_t>(initial_delay, sht3x_sensor_.get_initial_delay());
#endif

#ifdef CONFIG_SCD30_SENSOR_ENABLE
        scd30_sensor_.init();
        initial_delay = std::max<TickType_t>(initial_delay, scd30_sensor_.get_initial_delay());
#endif

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
        scd4x_sensor_.init();
        initial_delay = std::max<TickType_t>(initial_delay, scd4x_sensor_.get_initial_delay());
#endif

        // Wait until all sensors_ are ready
        vTaskDelay(initial_delay);

        do
        {
            read_bh1750_sensor();
            set_auto_display_brightness();
#ifdef CONFIG_SCD30_SENSOR_ENABLE
            read_scd30_sensor();
#endif

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
            read_scd4x_sensor();
#endif

#ifdef CONFIG_SHT3X_SENSOR_ENABLE
            read_sht3x_sensor();
#endif
            read_sps30_sensor();

            vTaskDelay(pdMS_TO_TICKS(sensor_history::sensor_interval / 20));
        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGE(OPERATIONS_TAG, "Hardware Task Failure:%s", ex.what());
        // throw;
    }

    vTaskDelete(NULL);
}

void hardware::read_bh1750_sensor()
{
    const auto values = bh1750_sensor_.read();

    if (!std::isnan(std::get<1>(values[0])))
    {
        light_sensor_values_.add_value(std::get<1>(values[0]));
    }

    const auto now = esp32::millis();
    if (now - bh1750_sensor_last_read_ >= sensor_history::sensor_interval)
    {
        for (auto &&value : values)
        {
            set_sensor_value(std::get<0>(value), std::get<1>(value));
        }

        bh1750_sensor_last_read_ = now;
    }
}

#ifdef CONFIG_SHT3X_SENSOR_ENABLE
void hardware::read_sht3x_sensor()
{
    read_sensor_if_time(sht3x_sensor_, sht3x_sensor_last_read_);
}
#endif

#ifdef CONFIG_SCD30_SENSOR_ENABLE
void hardware::read_scd30_sensor()
{
    read_sensor_if_time(scd30_sensor_, scd30_sensor_last_read_);
}
#endif

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
void hardware::read_scd4x_sensor()
{
    read_sensor_if_time(scd4x_sensor_, scd4x_sensor_last_read_);
}
#endif

void hardware::read_sps30_sensor()
{
    read_sensor_if_time(sps30_sensor_, sps30_sensor_last_read_);
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
    const auto config_brightness = config_.get_manual_screen_brightness();

    uint8_t required_brightness;
    if (config_brightness.has_value())
    {
        required_brightness = config_brightness.value();
        ESP_LOGD(SENSOR_BH1750_TAG, "Configured brightness:%d", required_brightness);
    }
    else
    {
        const auto avg_lux = light_sensor_values_.get_average();
        ESP_LOGD(SENSOR_BH1750_TAG, "Average lux:%g", avg_lux.value_or(128));
        required_brightness = lux_to_intensity(avg_lux.value_or(128));
    }

    ESP_LOGD(SENSOR_BH1750_TAG, "Required brightness:%d", required_brightness);
    display_.set_screen_brightness(required_brightness);
}

std::string hardware::get_sps30_error_register_status()
{
    return sps30_sensor_.get_error_register_status();
}
