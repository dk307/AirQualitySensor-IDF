#include "sdcard.h"
#include "logging/logging_tags.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"


#define SD_MISO GPIO_NUM_38
#define SD_MOSI GPIO_NUM_40
#define SD_SCLK GPIO_NUM_39
#define SD_CS GPIO_NUM_41

bool sd_card::pre_begin()
{
    sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    device_config.host_id = SPI3_HOST;
    device_config.gpio_cs = SD_CS;
    // device_config.gpio_cd = -1;   // SD Card detect

    ESP_LOGI(HARDWARE_TAG, "Initializing SD card");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = device_config.host_id;

    esp_vfs_fat_mount_config_t mount_config{};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 15;
    mount_config.allocation_unit_size = 16 * 1024;

    ESP_LOGI(HARDWARE_TAG, "Initializing SPI BUS");
    spi_bus_config_t bus_cfg{};
    bus_cfg.mosi_io_num = SD_MOSI;
    bus_cfg.miso_io_num = SD_MISO;
    bus_cfg.sclk_io_num = SD_SCLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4092;

    esp_err_t ret = spi_bus_initialize(device_config.host_id, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(HARDWARE_TAG, "Failed to initialize bus.");
        return false;
    }

    ESP_LOGI(HARDWARE_TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &device_config, &mount_config, &sdcard);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(HARDWARE_TAG, "Failed to mount filesystem. Probaly not formated");
            return false;
        }
        else
        {
            ESP_LOGE(HARDWARE_TAG, "Failed to initialize the card (%s)", esp_err_to_name(ret));
            return false;
        }
    }
    ESP_LOGI(HARDWARE_TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, sdcard);

    return true;
}