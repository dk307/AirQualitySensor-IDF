

#include "hardware/sensors/scd4x_sensor_device.h"

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
#include "hardware/pins.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <esp_log.h>
#include <i2cdev.h>

void scd4x_sensor_device::init()
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // needs 1000 ms to start working
    CHECK_THROW_ESP(scd4x_init_desc(&scd4x_sensor_, I2C_NUM_1, SDAWire, SCLWire));

    ESP_LOGI(SENSOR_SCD4x_TAG, "Initializing sensor...");
    CHECK_THROW_ESP(scd4x_stop_periodic_measurement(&scd4x_sensor_));
    CHECK_THROW_ESP(scd4x_reinit(&scd4x_sensor_));
    ESP_LOGI(SENSOR_SCD4x_TAG, "Sensor initialized");

    bool mal_function = false;
    CHECK_THROW_ESP(scd4x_perform_self_test(&scd4x_sensor_, &mal_function));
    if (mal_function)
    {
        CHECK_THROW_ESP2(ESP_FAIL, "SCD4x Self Test Failed");
    }
#ifdef CONFIG_SCD4x_SENSOR_TEMPERATURE_OFFSET
    CHECK_THROW_ESP(scd4x_set_temperature_offset(&scd4x_sensor_, CONFIG_SCD4x_SENSOR_TEMPERATURE_OFFSET / 100));
#endif
    CHECK_THROW_ESP(scd4x_set_automatic_self_calibration(&scd4x_sensor_, true));

    // low power mode to avoid additional heat generated
    CHECK_THROW_ESP(scd4x_start_low_power_periodic_measurement(&scd4x_sensor_));
}

std::array<std::tuple<sensor_id_index, float>, 4> scd4x_sensor_device::read()
{
    float co2 = NAN;
    float temperatureC = NAN;
    float temperatureF = NAN;
    float humidity = NAN;

    bool ready = false;
    auto error = scd4x_get_data_ready_status(&scd4x_sensor_, &ready);

    if ((error == ESP_OK))
    {
        if (ready)
        {
            uint16_t co2_int;
            auto err = scd4x_read_measurement(&scd4x_sensor_, &co2_int, &temperatureC, &humidity);
            if (err == ESP_OK)
            {
                co2 = co2_int;
                temperatureF = (temperatureC * 1.8) + 32;
                ESP_LOGI(SENSOR_SCD4x_TAG, "Read SCD40 sensor values:%g ppm %g F, %g C  %g %%", co2, temperatureF, temperatureC, humidity);
            }
            else
            {
                ESP_LOGE(SENSOR_SCD4x_TAG, "Failed to read from SCD40 sensor with error:%s", esp_err_to_name(err));
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
        ESP_LOGE(SENSOR_SCD4x_TAG, "Failed to read from SCD40 sensor with failed to read measurement error:0x%x", error);
    }

    return {std::tuple<sensor_id_index, float>{sensor_id_index::temperatureC, esp32::round_with_precision(temperatureC, 0.01)},
            std::tuple<sensor_id_index, float>{sensor_id_index::temperatureF, esp32::round_with_precision(temperatureF, 0.1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::CO2, esp32::round_with_precision(co2, 1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::humidity, esp32::round_with_precision(humidity, 1)}};
}

uint8_t scd4x_sensor_device::get_initial_delay()
{
    return 0;
}
#endif