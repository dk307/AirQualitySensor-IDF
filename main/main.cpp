#include "app_events.h"
#include "config/config_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware/hardware.h"
#include "hardware/sd_card.h"
#include "homekit/homekit_integration.h"
#include "logging/logger.h"
#include "logging/logging_tags.h"
#include "sdkconfig.h"
#include "util/exceptions.h"
#include "web_server/web_server.h"
#include "wifi/wifi_manager.h"
#include <esp_flash.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <stdio.h>

ESP_EVENT_DEFINE_BASE(APP_COMMON_EVENT);

extern "C" void app_main(void)
{
    ESP_LOGI(OPERATIONS_TAG, "Starting ....");
    esp_log_level_set("*", ESP_LOG_NONE);
    esp_log_level_set(HOMEKIT_TAG, ESP_LOG_DEBUG);

    try
    {
        const auto err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            CHECK_THROW_ESP(nvs_flash_erase());
            CHECK_THROW_ESP(nvs_flash_init());
        }

        CHECK_THROW_ESP(esp_event_loop_create_default());

        // order is important
        sd_card::instance.begin();
        config::instance.begin();
        hardware::instance.begin();
        wifi_manager::instance.begin();
        web_server::instance.begin();
        homekit_integration::instance.begin();
        operations::mark_running_parition_as_valid();

        CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, APP_INIT_DONE));

        ESP_LOGI(OPERATIONS_TAG, "Main task is done");
    }
    catch (const std::exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "Init Failure:%s", ex.what());
        operations::try_mark_running_parition_as_invalid();
        throw;
    }
}
