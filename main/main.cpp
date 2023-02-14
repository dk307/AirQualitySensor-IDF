#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "logging/logging_tags.h"


#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_partition.h"



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

    ESP_LOGI(OPERATIONS_TAG, "%uMB %s flash\n", flash_size / (1024 * 1024),
          (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ESP_LOGI(OPERATIONS_TAG, "Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
    // log_i("SPIRAM size: %d bytes\n", esp_spiram_get_size());
    return true;
}

extern "C" void app_main(void)
{
    ESP_LOGI(OPERATIONS_TAG, "Starting ...");
    log_chip_details();

    for (int i = 10; i >= 0; i--)
    {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
