#pragma once

#include <esp_partition.h>

namespace esp32
{
#include <cstring>
#include <stdexcept>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

    static const char *TAG = "OTAUpdater";

#include <cstring>
#include <stdexcept>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

    static const char *TAG = "OTAUpdater";

    class OTAUpdater
    {
    public:
        OTAUpdater(const uint8_t *expected_sha256) : handle_(NULL), expected_sha256_(expected_sha256) {}

        void start()
        {
            ESP_LOGI(TAG, "OTA update started");

            const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
            if (update_partition == NULL)
            {
                ESP_LOGE(TAG, "Failed to get OTA update partition");
                throw std::runtime_error("Failed to get OTA update partition");
            }

            handle_ = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &handle_);
            if (handle_ == NULL)
            {
                ESP_LOGE(TAG, "Failed to begin OTA update");
                throw std::runtime_error("Failed to begin OTA update");
            }
        }

        void write(const uint8_t *data, size_t size)
        {
            esp_err_t ret = esp_ota_write(handle_, data, size);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to write OTA data (%d)", ret);
                throw std::runtime_error("Failed to write OTA data");
            }
        }

        void end()
        {
            ESP_LOGI(TAG, "OTA update completed");

            esp_err_t ret = esp_ota_end(handle_);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Firmware update failed to complete (%d)", ret);
                throw std::runtime_error("Firmware update failed to complete");
            }

            const esp_partition_t *updated_partition = esp_ota_get_next_update_partition(NULL);

            uint8_t actual_sha256[32];
            ret = esp_partition_get_sha256(updated_partition, actual_sha256);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "SHA-256 checksum verification failed (%d)", ret);
                throw std::runtime_error("SHA-256 checksum verification failed");
            }

            if (memcmp(expected_sha256_, actual_sha256, 32) != 0)
            {
                ESP_LOGE(TAG, "SHA-256 checksum does not match expected value");
                throw std::runtime_error("SHA-256 checksum does not match expected value");
            }

            if (esp_partition_check_checksum(updated_partition) != ESP_OK)
            {
                ESP_LOGE(TAG, "OTA update partition is corrupted");
                throw std::runtime_error("OTA update partition is corrupted");
            }

            ret = esp_ota_set_boot_partition(updated_partition);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set boot partition (%d)", ret);
                throw std::runtime_error("Failed to set boot partition");
            }

            ESP_LOGI(TAG, "OTA update successfully installed");
        }

        void abort()
        {
            ESP_LOGI(TAG, "OTA update aborted");

            esp_err_t ret = esp_ota_abort(handle_);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to abort OTA update (%d)", ret);
                throw std::runtime_error("Failed to abort OTA update");
            }
        }

        bool isRunning()
        {
            return esp_ota_get_state() == OTA_STATE_IN_PROGRESS;
        }

        bool hasErrored()
        {
            return esp_ota_get_last_error() != ESP_OK;
        }

    private:
        esp_ota_handle_t handle_;
        const uint8_t *expected_sha256_;
    };

} // namespace esphome
