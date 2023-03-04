#include "sps30.h"

#include "hardware/hardware.h"
#include "logging/logging_tags.h"

#include <esp_log.h>
#include <esp_task.h>
#include <i2cdev.h>

extern "C" int8_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count)
{
    auto error = hardware::instance.sensirion_i2c_read(address, data, count);
    if (error != ESP_OK)
    {
        ESP_LOGW(SENSOR_SPS30_TAG, "I2C Read Operation Failed with:%s", esp_err_to_name(error));
        return 1;
    }
    return 0;
}

extern "C" int8_t sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count)
{
    auto error = hardware::instance.sensirion_i2c_write(address, data, count);
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