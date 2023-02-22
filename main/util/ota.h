#pragma once

#include <esp_partition.h>

namespace esp32
{
    class OTA
    {
    public:
        bool begin(int image_size);
        bool is_running() const { return this->handle_; }
        void abort();
        bool hasError() const { return this->error_ != ESP_OK; }
        bool write(const uint8_t *data, int size);
        bool end();

        esp_err_t get_error() const { return this->error_; }

    protected:
        const esp_partition_t *partition_{};
        esp_ota_handle_t handle_{};
        esp_err_t error_{ESP_OK};
    };

} // namespace esphome
