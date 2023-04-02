

#include "hardware/sensors/sht3x_sensor_device.h"
#include "hardware/pins.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <esp_log.h>
#include <i2cdev.h>
#include <sht3x.h>

void sht3x_sensor_device::init()
{
    // sht3x
    CHECK_THROW_ESP(sht3x_init_desc(&sht3x_sensor, SHT3X_I2C_ADDR_GND, I2C_NUM_1, SDAWire, SCLWire));
    CHECK_THROW_ESP(sht3x_init(&sht3x_sensor));
}

std::array<std::tuple<sensor_id_index, float>, 3> sht3x_sensor_device::read()
{
    float temperatureC = NAN;
    float humidity = NAN;
    auto err = sht3x_measure(&sht3x_sensor, &temperatureC, &humidity);
    float temperatureF = NAN;
    if (err == ESP_OK)
    {
        temperatureF = (temperatureC * 1.8) + 32;
        ESP_LOGI(SENSOR_SHT31_TAG, "Read SHT31 sensor values:%g F, %g C  %g %%", temperatureF, temperatureC, humidity);
    }
    else
    {
        ESP_LOGE(SENSOR_SHT31_TAG, "Failed to read from SHT3x sensor with error:%s", esp_err_to_name(err));
    }

    return {std::tuple<sensor_id_index, float>{sensor_id_index::temperatureC, esp32::round_with_precision(temperatureC, 0.01)},
            std::tuple<sensor_id_index, float>{sensor_id_index::temperatureF, esp32::round_with_precision(temperatureF, 0.1)},
            std::tuple<sensor_id_index, float>{sensor_id_index::humidity, esp32::round_with_precision(humidity, 1)}};
}