#include "operations.h"

#include "config/config_manager.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <nvs_flash.h>

operations operations::instance;

operations::operations()
{
    esp_register_shutdown_handler(shutdown_restarting);
}

void operations::shutdown_restarting()
{
    operations::instance.reset_pending.store(true);
}

void operations::factory_reset()
{
    ESP_LOGW(OPERATIONS_TAG, "Doing Factory Reset");
    nvs_flash_erase();
    config::erase();
    reset();
}

void operations::reboot()
{
    reset();
}

[[noreturn]] void operations::reset()
{
    ESP_LOGI(OPERATIONS_TAG, "Restarting...");
    reset_pending.store(true);
    vTaskDelay(pdMS_TO_TICKS(2000)); // for http response, etc to finish
    esp_restart();
}

void operations::mark_running_parition_as_valid()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state{};

    auto error = esp_ota_get_state_partition(running, &ota_state);

    CHECK_THROW_INIT(error, "esp_ota_get_state_partition for running parition failed");

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGI(OPERATIONS_TAG, "Marking running partition as valid");
        error = esp_ota_mark_app_valid_cancel_rollback();
        CHECK_THROW_INIT(error, "Failed to mark running parition as VALID");
    }
    else
    {
        ESP_LOGI(OPERATIONS_TAG, "No need to mark running partition as valid as it is already %d state", ota_state);
    }
}

bool operations::try_mark_running_parition_as_invalid()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state{};

    auto error = esp_ota_get_state_partition(running, &ota_state);

    if (error == ESP_OK)
    {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            error = esp_ota_mark_app_invalid_rollback_and_reboot();
            if (error != ESP_OK)
            {
                ESP_LOGI(OPERATIONS_TAG, "Failed to get running partition as invalid with %s", esp_err_to_name(error));
            }
            else
            {
                ESP_LOGI(OPERATIONS_TAG, "Marked running partition as INVALID");
                return true;
            }
        }
    }
    else
    {
        ESP_LOGI(OPERATIONS_TAG, "Failed to get running partition state with %s", esp_err_to_name(error));
    }
    return false;
}