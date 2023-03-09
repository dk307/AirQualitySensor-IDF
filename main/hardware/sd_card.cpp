#include "sd_card.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include <esp_log.h>
#include <esp_vfs_fat.h>

#define SD_MISO GPIO_NUM_38
#define SD_MOSI GPIO_NUM_40
#define SD_SCLK GPIO_NUM_39
#define SD_CS GPIO_NUM_41

sd_card sd_card::instance;

void sd_card::begin()
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
    mount_config.allocation_unit_size = 512;
    // mount_config.disk_status_check_enable = true;

    ESP_LOGI(HARDWARE_TAG, "Initializing SPI BUS");
    spi_bus_config_t bus_cfg{};
    bus_cfg.mosi_io_num = SD_MOSI;
    bus_cfg.miso_io_num = SD_MISO;
    bus_cfg.sclk_io_num = SD_SCLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4092;

    CHECK_THROW_ESP(spi_bus_initialize(device_config.host_id, &bus_cfg, SDSPI_DEFAULT_DMA));

    ESP_LOGI(HARDWARE_TAG, "Mounting filesystem");
    CHECK_THROW_ESP(esp_vfs_fat_sdspi_mount(mount_point, &host, &device_config, &mount_config, &sd_card_));
    ESP_LOGI(HARDWARE_TAG, "Filesystem mounted");

    ESP_LOGI(HARDWARE_TAG, "%s", get_info().c_str());
}

std::string sd_card::get_info()
{
    std::string info;
    info.reserve(256);

    const char *type;
    info += "Name: " + std::string(sd_card_->cid.name) + "\n";
    if (sd_card_->is_sdio)
    {
        type = "SDIO";
    }
    else if (sd_card_->is_mmc)
    {
        type = "MMC";
    }
    else
    {
#define SD_OCR_SDHC_CAP (1 << 30)
        type = (sd_card_->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC";
    }

    info += "Type: " + std::string(type) + "\n";
    if (sd_card_->max_freq_khz < 1000)
    {
        info += "Speed: " + esp32::string::to_string(sd_card_->max_freq_khz) + " kHz\n";
    }
    else
    {
        info += "Speed: " + esp32::string::to_string(sd_card_->max_freq_khz / 1000) + " kHz\n";
        ;
        if (sd_card_->is_ddr)
        {
            info += ", DDR";
        }
        info += "\n";
    }

    info += "Size: " + esp32::string::stringify_size(static_cast<uint64_t>(sd_card_->csd.capacity) * sd_card_->csd.sector_size) + "\n";

    return info;
}
