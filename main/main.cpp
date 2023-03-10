#include "app_events.h"
#include "config/config_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hardware/hardware.h"
#include "hardware/sd_card.h"
#include "http_ota/http.h"
#include "logging/logging.h"
#include "logging/logging_tags.h"
#include "sdkconfig.h"
#include "util/exceptions.h"
#include "web_server/web_server.h"
#include "wifi/wifi_manager.h"
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <stdio.h>

ESP_EVENT_DEFINE_BASE(APP_COMMON_EVENT);

// #define MIN_FOR_UPLOAD

extern "C" void app_main(void)
{
    ESP_LOGD(OPERATIONS_TAG, "Starting ....");

    try
    {
        auto err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            CHECK_THROW_ESP(nvs_flash_erase());
            CHECK_THROW_ESP(nvs_flash_init());
        }

        CHECK_THROW_ESP(esp_event_loop_create_default());

        // order is important
        sd_card::instance.begin();

        config::instance.begin();
#ifndef MIN_FOR_UPLOAD
        hardware::instance.begin();
#endif
        wifi_manager::instance.begin();

#ifndef MIN_FOR_UPLOAD
        web_server::instance.begin();
#endif
        operations::mark_running_parition_as_valid();

#ifdef MIN_FOR_UPLOAD
        http_init();
        httpd_handle_t http_server;
        http_start_webserver(&http_server);
#endif

        CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, APP_INIT_DONE));

        ESP_LOGI(OPERATIONS_TAG, "Main task is done!");
    }
    catch (const std::exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "Init Failure:%s", ex.what());
        operations::try_mark_running_parition_as_invalid();
        throw;
     }
}
