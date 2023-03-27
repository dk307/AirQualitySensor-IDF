#include "operations.h"
#include "app_events.h"
#include "config/config_manager.h"
#include "logging/logging_tags.h"
#include "util/default_event.h"
#include "util/exceptions.h"
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <nvs.h>
#include <nvs_flash.h>

operations operations::instance;

operations::operations()
{
    esp_register_shutdown_handler(operations_shutdown_handler);
}

void operations::operations_shutdown_handler()
{
    operations::instance.reset_pending.store(true);
}

#include "nvs_flash.h"

// Function to erase all NVS partitions
void operations::erase_all_nvs_partitions()
{
    auto it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);

    for (; it != NULL; it = esp_partition_next(it))
    {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGW(OPERATIONS_TAG, "Erasing partition:%s", part->label);
        nvs_flash_erase_partition(part->label);
    }
    esp_partition_iterator_release(it);
}

void operations::factory_reset()
{
    ESP_LOGW(OPERATIONS_TAG, "Doing Factory Reset");
    erase_all_nvs_partitions();
    reboot_timer_no_exception(3);
}

void operations::reboot()
{
    ESP_LOGI(OPERATIONS_TAG, "Restarting...");
    reboot_timer_no_exception(2);
}

void operations::mark_running_parition_as_valid()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state{};

    CHECK_THROW_ESP(esp_ota_get_state_partition(running, &ota_state));

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGI(OPERATIONS_TAG, "Marking running partition as valid");
        CHECK_THROW_ESP(esp_ota_mark_app_valid_cancel_rollback());
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

void operations::reboot_timer_no_exception(uint8_t seconds)
{
    try
    {
        reboot_timer(seconds);
    }
    catch (...)
    {
        esp_restart();
    }
}

void operations::reboot_timer(uint8_t seconds)
{
    reset_pending.store(true);

    /* If specified time is 0, reboot immediately */
    if (seconds == 0)
    {
        CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, APP_EVENT_REBOOT, seconds));
        esp_restart();
        return;
    }
    else if (reboot_timer_)
    {
        return;
    }

    reboot_timer_ = std::make_unique<esp32::timer::timer>(
        [] {
            ESP_LOGI(OPERATIONS_TAG, "Restarting now");
            esp_restart();
        },
        "reboot_timer");
    reboot_timer_->start_one_shot(std::chrono::seconds(seconds));
    CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, APP_EVENT_REBOOT, seconds));
    return;
}