#include "app_events.h"
#include "config/config_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware/display/display.h"
#include "hardware/hardware.h"
#include "homekit/homekit_integration.h"
#include "logging/logger.h"
#include "logging/logging_tags.h"
#include "operations/operations.h"
#include "sdkconfig.h"
#include "util/exceptions.h"
#include "web_server/web_server.h"
#include "wifi/wifi_manager.h"
#include <esp_flash.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <stdio.h>

#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
#include "hardware/sd_card.h"
#endif

ESP_EVENT_DEFINE_BASE(APP_COMMON_EVENT);

extern "C" void app_main(void)
{
    ESP_LOGI(OPERATIONS_TAG, "Starting ....");
    esp_log_level_set("*", ESP_LOG_NONE);

    try
    {
        const auto err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_LOGW(OPERATIONS_TAG, "Erasing flash");
            CHECK_THROW_ESP(nvs_flash_erase());
            CHECK_THROW_ESP(nvs_flash_init());
        }

        CHECK_THROW_ESP(esp_event_loop_create_default());

        auto &config = config::create_instance();
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
        auto &sd_card = sd_card::create_instance();
#endif
        auto &wifi_manager = wifi_manager::create_instance(config);
        auto &ui_interface = ui_interface::create_instance();
        auto &display = display::create_instance(config, ui_interface);
        auto &hardware = hardware::create_instance(config, display);
        auto &homekit_integration = homekit_integration::create_instance(config, hardware);
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
        auto &logger = logger::create_instance(sd_card);
#else
        auto &logger = logger::create_instance();
#endif
        auto &web_server = web_server::create_instance(config, ui_interface, logger);

        // update later to account for circular dependency
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
        ui_interface.update(config, sd_card, hardware, wifi_manager, homekit_integration);
#else
        ui_interface.update(config, hardware, wifi_manager, homekit_integration);
#endif
        // order is important
        config.begin();
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
        sd_card.begin();
#endif
        display.begin();
        wifi_manager.begin();
        hardware.begin();
        web_server.begin();
        homekit_integration.begin();

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
