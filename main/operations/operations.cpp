#include "operations.h"

#include "config/config_manager.h"
#include "logging/logging_tags.h"

#include <esp_log.h>
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