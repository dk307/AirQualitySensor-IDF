

#include "hardware/sensors/bh1750_sensor_device.h"
#include "hardware/pins.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "util/noncopyable.h"
#include <bh1750.h>
#include <esp_log.h>
#include <i2cdev.h>


void bh1750_sensor_device::init()
{
    CHECK_THROW_ESP(bh1750_init_desc(&bh1750_sensor, BH1750_ADDR_LO, I2C_NUM_1, SDAWire, SCLWire));
    CHECK_THROW_ESP(bh1750_setup(&bh1750_sensor, BH1750_MODE_CONTINUOUS, BH1750_RES_HIGH));
}

std::array<std::tuple<sensor_id_index, float>, 1> bh1750_sensor_device::read()
{
    float lux = NAN;
    uint16_t level_lux = 0;
    const auto err = bh1750_read(&bh1750_sensor, &level_lux);
    if (err != ESP_OK)
    {
        ESP_LOGW(SENSOR_BH1750_TAG, "Failed to read sensor with %s", esp_err_to_name(err));
    }
    else
    {
        lux = level_lux;
    }

    return {std::tuple<sensor_id_index, float>{sensor_id_index::light_intensity, esp32::round_with_precision(lux, 1)}};
}