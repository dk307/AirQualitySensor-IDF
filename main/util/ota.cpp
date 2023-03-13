#include "util/ota.h"

#include "logging/logging_tags.h"
#include <cstring>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <stdexcept>

namespace esp32
{

#define CHECK_THROW_OTA(error_, message) CHECK_THROW(error_, ota_exception)

ota_updator::ota_updator(const std::array<uint8_t, 32> &expected_sha256) : expected_sha256_(expected_sha256)
{
    ESP_LOGI(OPERATIONS_TAG, "OTA update started");

    update_partition_ = esp_ota_get_next_update_partition(NULL);
    if (update_partition_ == NULL)
    {
        CHECK_THROW_OTA(ESP_FAIL, "Failed to get OTA update partition");
    }

    ESP_LOGI(OPERATIONS_TAG, "Writing partition: type %d, subtype %d, offset 0x%lx\n", update_partition_->type, update_partition_->subtype,
             update_partition_->address);

    auto ret = esp_ota_begin(update_partition_, OTA_SIZE_UNKNOWN, &handle_);
    CHECK_THROW_OTA(ret, "Failed to begin OTA update");
}

void ota_updator::write(const uint8_t *data, size_t size)
{
    if (!handle_)
    {
        CHECK_THROW_OTA(ESP_ERR_NOT_SUPPORTED, "No OTA in progress");
    }

    const esp_err_t ret = esp_ota_write(handle_, data, size);
    CHECK_THROW_OTA(ret, "Failed to write OTA data");
}

esp_err_t ota_updator::write2(const uint8_t *data, size_t size) noexcept
{
    return esp_ota_write(handle_, data, size);
}

void ota_updator::end()
{
    if (!handle_)
    {
        CHECK_THROW_OTA(ESP_ERR_NOT_SUPPORTED, "No OTA in progress");
    }

    ESP_LOGI(OPERATIONS_TAG, "OTA update completed");

    esp_err_t ret = esp_ota_end(handle_);
    CHECK_THROW_OTA(ret, "Failed to end OTA update");
    handle_ = 0;

    uint8_t actual_sha256[32]{};
    ret = esp_partition_get_sha256(update_partition_, actual_sha256);
    CHECK_THROW_OTA(ret, "Getting SHA-256 of update failed");

    if (memcmp(expected_sha256_.data(), actual_sha256, 32) != 0)
    {
        CHECK_THROW_OTA(ESP_FAIL, "SHA-256 does not match as expected");
    }
    else
    {
        ESP_LOGI(OPERATIONS_TAG, "SHA256 match after ota");
    }

    ret = esp_ota_set_boot_partition(update_partition_);
    CHECK_THROW_OTA(ret, "Setting bootable partition failed");

    ESP_LOGI(OPERATIONS_TAG, "OTA update successfully updated");
}

void ota_updator::abort()
{
    ESP_LOGI(OPERATIONS_TAG, "OTA update aborted");

    esp_err_t ret = esp_ota_abort(handle_);
    CHECK_THROW_OTA(ret, "Failed to abort update");
    handle_ = 0;
}

bool ota_updator::is_running()
{
    return handle_ != 0;
}
} // namespace esp32
