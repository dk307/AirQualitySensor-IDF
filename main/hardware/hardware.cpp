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
    return (*sensors_history)[static_cast<size_t>(index)].get_snapshot(sensor_history::reads_per_minute);
}

bool hardware::clean_sps_30()
{
    return sps30_sensor.clean();
}

void hardware::begin()
{
    CHECK_THROW_ESP(i2cdev_init());
    sensor_refresh_task.spawn_pinned("sensor_task", 4 * 1024, esp32::task::default_priority, esp32::hardware_core);
}

void hardware::set_sensor_value(sensor_id_index index, float value)
{
    bool changed;
    const auto i = static_cast<size_t>(index);
    if (!std::isnan(value))
    {
        (*sensors_history)[i].add_value(value);
        changed = sensors[i].set_value(value);
        ESP_LOGI(HARDWARE_TAG, "Updated for sensor:%.*s Value:%g", get_sensor_name(index).size(), get_sensor_name(index).data(),
                 sensors[i].get_value());
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

        TickType_t initial_delay = 0;

        sps30_sensor.init();
        bh1750_sensor.init();
        initial_delay = std::max(initial_delay, bh1750_sensor.get_initial_delay());

#ifdef CONFIG_SHT3X_SENSOR_ENABLE
        sht3x_sensor.init();
        initial_delay = std::max<TickType_t>(initial_delay, sht3x_sensor.get_initial_delay());
#endif

        // Wait until all sensors are ready
        vTaskDelay(initial_delay);

        do
        {
            read_bh1750_sensor();
            set_auto_display_brightness();
#ifdef CONFIG_SHT3X_SENSOR_ENABLE
            read_sht3x_sensor();
#endif
            read_sps30_sensor();

            vTaskDelay(pdMS_TO_TICKS(500));
        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGE(OPERATIONS_TAG, "Hardware Task Failure:%s", ex.what());
        throw;
    }

    vTaskDelete(NULL);
}

void hardware::read_bh1750_sensor()
{
    const auto values = bh1750_sensor.read();

    if (!std::isnan(std::get<1>(values[0])))
    {
        light_sensor_values.add_value(std::get<1>(values[0]));
    }

    const auto now = esp32::millis();
    if (now - bh1750_sensor_last_read >= sensor_history::sensor_interval)
    {
        for (auto &&value : values)
        {
            set_sensor_value(std::get<0>(value), std::get<1>(value));
        }

        bh1750_sensor_last_read = now;
    }
}

#ifdef CONFIG_SHT3X_SENSOR_ENABLE
void hardware::read_sht3x_sensor()
{
    read_sensor_if_time(sht3x_sensor, sht3x_sensor_last_read);
}
#endif

void hardware::read_sps30_sensor()
{
    read_sensor_if_time(sps30_sensor, sps30_sensor_last_read);
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
        const auto avg_lux = light_sensor_values.get_average();
        ESP_LOGD(SENSOR_BH1750_TAG, "Average lux:%g", avg_lux.value_or(128));
        required_brightness = lux_to_intensity(avg_lux.value_or(128));
    }

    ESP_LOGD(SENSOR_BH1750_TAG, "Required brightness:%d", required_brightness);
    display_.set_screen_brightness(required_brightness);
}

std::string hardware::get_sps30_error_register_status()
{
    return sps30_sensor.get_error_register_status();
}
