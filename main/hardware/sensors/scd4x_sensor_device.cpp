

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
    vTaskDelay(pdMS_TO_TICKS(1000)); //needs 1000 ms to start working
    CHECK_THROW_ESP(scd4x_init_desc(&scd4x_sensor_, I2C_NUM_1, SDAWire, SCLWire));
    CHECK_THROW_ESP(scd4x_start_periodic_measurement(&scd4x_sensor_));
    CHECK_THROW_ESP(scd4x_set_automatic_self_calibration(&scd4x_sensor_, true));
}

std::array<std::tuple<sensor_id_index, float>, 4> scd4x_sensor_device::read()
{
    float co2 = NAN;
    float temperatureC = NAN;
    float temperatureF = NAN;
    float humidity = NAN;
    const bool data_ready = wait_till_data_ready();

    if (data_ready)
    {
        uint16_t co2_uint{0};
        auto err = scd4x_read_measurement(&scd4x_sensor_, &co2_uint, &temperatureC, &humidity);
        if (err == ESP_OK)
        {
            co2 = co2_uint;
#ifdef CONFIG_SCD4X_SENSOR_TEMPERATURE_OFFSET
            temperatureC -= CONFIG_SCD4X_SENSOR_TEMPERATURE_OFFSET / 100; // offset for the heat generated by sensor itself.
#endif
            temperatureF = (temperatureC * 1.8) + 32;
            ESP_LOGI(SENSOR_SCD4x_TAG, "Read SCD4X sensor values:%g ppm %g F, %g C  %g %%", co2, temperatureF, temperatureC, humidity);
        }
        else
        {
            ESP_LOGE(SENSOR_SCD4x_TAG, "Failed to read from SCD4X sensor with error:%s", esp_err_to_name(err));
        }
    }

    return {std::tuple<sensor_id_index, float>{sensor_id_index::temperatureC, esp32::round_with_precision(temperatureC, 0.01)},
            std::tuple<sensor_id_index, float>{sensor_id_index::temperatureF, esp32::round_with_precision(temperatureF, 0.1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::CO2, esp32::round_with_precision(co2, 1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::humidity, esp32::round_with_precision(humidity, 1)}};
}

bool scd4x_sensor_device::wait_till_data_ready()
{
    uint8_t retries = 0;
    bool ready = false;

    do
    {
        auto error = scd4x_get_data_ready_status(&scd4x_sensor_, &ready);

        if ((error == ESP_OK))
        {
            if (ready)
            {
                return true;
            }
            else
            {
                vTaskDelay(max_wait_ticks_ / 5);
                retries++;
            }
        }
        else
        {
            ESP_LOGE(SENSOR_SCD4x_TAG, "Failed to read from SPS30 sensor with measurement error:0x%x", error);
            return false;
        }

    } while (retries < 5);
    return false;
}

uint8_t scd4x_sensor_device::get_initial_delay()
{
    return 0;
}
#endif