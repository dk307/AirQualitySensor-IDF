#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "logging/logging_tags.h"
#include "hardware/sdcard.h"
#include "config/config_manager.h"
#include "hardware/hardware.h"
#include "wifi/wifi_manager.h"
#include "web_server/web_server.h"
#include "exceptions.h"
#include "http_ota/http.h"

#include <esp_log.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <nvs_flash.h>

bool log_chip_details()
{
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    ESP_LOGI(OPERATIONS_TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
             CONFIG_IDF_TARGET,
             chip_info.cores,
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    ESP_LOGI(OPERATIONS_TAG, "silicon revision v%d.%d, ", major_rev, minor_rev);
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        ESP_LOGE(OPERATIONS_TAG, "Get flash size failed");
        return false;
    }

    ESP_LOGI(OPERATIONS_TAG, "%lu MB %s flash\n", flash_size / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(OPERATIONS_TAG, "Minimum free heap size: %ld bytes\n", esp_get_free_heap_size());
    // log_i("SPIRAM size: %d bytes\n", esp_spiram_get_size());
    return true;
}

sd_card card;

extern "C" void app_main(void)
{
    esp_log_level_set(CONFIG_TAG, ESP_LOG_DEBUG);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    try
    {
        ESP_LOGI(WEBSERVER_TAG, "Starting ...");
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        // log_chip_details();

        card.pre_begin();
        config::instance.pre_begin();

        // hardware::instance.pre_begin();

        wifi_manager::instance.begin();

        while (!wifi_manager::instance.is_wifi_connected())
        {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        http_init();
        httpd_handle_t http_server;
        http_start_webserver(&http_server);

        web_server::instance.begin();

        // hardware::instance.begin();

        // hardware::instance.set_main_screen();

        ESP_LOGI(OPERATIONS_TAG, "Minimum free heap size: %ld bytes\n", esp_get_free_heap_size());
    }
    catch (const esp32::init_failure_exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "Init Failure:%s", ex.what());
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    }
}
