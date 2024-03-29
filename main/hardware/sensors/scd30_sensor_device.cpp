

#include "hardware/sensors/scd30_sensor_device.h"

#ifdef CONFIG_SCD30_SENSOR_ENABLE
#include "hardware/pins.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <esp_log.h>
#include <i2cdev.h>

void scd30_sensor_device::init()
{
    CHECK_THROW_ESP(scd30_init_desc(&scd30_sensor_, I2C_NUM_1, SDAWire, SCLWire));
    CHECK_THROW_ESP(scd30_set_temperature_offset(&scd30_sensor_, CONFIG_SCD30_SENSOR_TEMPERATURE_OFFSET / 100));
    constexpr uint16_t interval = (decltype(last_measurement_value_)::max_value_interval_ms / 1000) / 2;
    ESP_LOGI(SENSOR_SCD30_TAG, "Interval is :%u seconds", interval);
    CHECK_THROW_ESP(scd30_set_measurement_interval(&scd30_sensor_, interval));
    CHECK_THROW_ESP(scd30_set_automatic_self_calibration(&scd30_sensor_, true));
}

std::array<std::tuple<sensor_id_index, float>, 4> scd30_sensor_device::read()
{
    float co2 = NAN;
    float temperatureC = NAN;
    float temperatureF = NAN;
    float humidity = NAN;

    bool ready = false;
    auto error = scd30_get_data_ready_status(&scd30_sensor_, &ready);

    if ((error == ESP_OK))
    {
        if (ready)
        {
            auto err = scd30_read_measurement(&scd30_sensor_, &co2, &temperatureC, &humidity);
            if (err == ESP_OK)
            {
                temperatureF = (temperatureC * 1.8) + 32;
                ESP_LOGI(SENSOR_SCD30_TAG, "Read SCD30 sensor values:%g ppm %g F, %g C  %g %%", co2, temperatureF, temperatureC, humidity);
            }
            else
            {
                ESP_LOGE(SENSOR_SCD30_TAG, "Failed to read from SCD30 sensor with error:%s", esp_err_to_name(err));
            }
            last_measurement_value_.store(std::tie(co2, temperatureC, temperatureF, humidity));
        }
        else
        {
            std::tuple<float, float, float, float> old_values;
            if (last_measurement_value_.update(old_values))
            {
                std::tie(co2, temperatureC, temperatureF, humidity) = old_values;
            }
        }
    }
    else
    {
        ESP_LOGE(SENSOR_SCD30_TAG, "Failed to read from SCD30 sensor with failed to read measurement error:0x%x", error);
    }

    return {std::tuple<sensor_id_index, float>{sensor_id_index::temperatureC, esp32::round_with_precision(temperatureC, 0.01)},
            std::tuple<sensor_id_index, float>{sensor_id_index::temperatureF, esp32::round_with_precision(temperatureF, 0.1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::CO2, esp32::round_with_precision(co2, 1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::humidity, esp32::round_with_precision(humidity, 1)}};
}

uint8_t scd30_sensor_device::get_initial_delay()
{
    return 0;
}
#endif