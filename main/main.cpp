#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

#include "config/config_manager.h"
#include "hardware/hardware.h"
#include "hardware/sd_card.h"
#include "http_ota/http.h"
#include "logging/logging.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "web_server/web_server.h"
#include "wifi/wifi_manager.h"

#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_log.h>
#include <nvs_flash.h>

// #define MIN_FOR_UPLOAD

extern "C" void app_main(void)
{
    // esp_log_level_set(WIFI_TAG, ESP_LOG_DEBUG);
    // esp_log_level_set("wifi:Init", ESP_LOG_DEBUG);
    // esp_log_level_set("wpa", ESP_LOG_DEBUG);
    esp_log_level_set("sensor_sps30", ESP_LOG_ERROR);
    esp_log_level_set("sensor_sht31", ESP_LOG_ERROR);
    esp_log_level_set("sensor_bh1750", ESP_LOG_ERROR);
    esp_log_level_set("webserver", ESP_LOG_DEBUG);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGD(OPERATIONS_TAG, "Starting ....");

    try
    {
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

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

#ifndef MIN_FOR_UPLOAD
        hardware::instance.set_main_screen();
#endif

        ESP_LOGI(OPERATIONS_TAG, "Main task is done!");
    }
    catch (const std::exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "Init Failure:%s", ex.what());
        operations::try_mark_running_parition_as_invalid();
        vTaskDelay(pdMS_TO_TICKS(5000));
        operations::instance.reboot();
    }
}
