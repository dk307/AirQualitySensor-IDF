#include "sdcard.h"
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

void sd_card::pre_begin()
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

    esp_err_t ret = spi_bus_initialize(device_config.host_id, &bus_cfg, SDSPI_DEFAULT_DMA);
    CHECK_THROW(ret, "Failed to initialize SPI bus", esp32::init_failure_exception);

    ESP_LOGI(HARDWARE_TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &device_config, &mount_config, &sd_card_);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            CHECK_THROW(ret, "Failed to mount filesystem. Likely not formated", esp32::init_failure_exception);
        }
        else
        {
            CHECK_THROW(ret, "Failed to initialize the card", esp32::init_failure_exception);
        }
    }
    ESP_LOGI(HARDWARE_TAG, "Filesystem mounted");

    ESP_LOGI(HARDWARE_TAG, "%s", get_info().c_str());
}

std::string sd_card::get_info()
{
    std::string info;
    info.reserve(256);

    bool print_scr = false;
    bool print_csd = false;
    const char *type;
    info += "Name: " + std::string(sd_card_->cid.name) + "\n";
    if (sd_card_->is_sdio)
    {
        type = "SDIO";
        print_scr = true;
        print_csd = true;
    }
    else if (sd_card_->is_mmc)
    {
        type = "MMC";
        print_csd = true;
    }
    else
    {
        #define SD_OCR_SDHC_CAP                 (1<<30)
        type = (sd_card_->ocr & SD_OCR_SDHC_CAP) ? "SDHC/SDXC" : "SDSC";
        print_csd = true;
    }
    info += "Type: " + std::string(type) + "\n";
    if (sd_card_->max_freq_khz < 1000)
    {
        info += "Speed: " + std::to_string(sd_card_->max_freq_khz) + " kHz\n";
    }
    else
    {
        info += "Speed: " + std::to_string(sd_card_->max_freq_khz / 1000);
        if (sd_card_->is_ddr)
        {
            info += ", DDR";
        }
        info += "\n";
    }

    info += "Size: " + std::to_string(((uint64_t)sd_card_->csd.capacity) * sd_card_->csd.sector_size / (1024 * 1024)) + "MB\n";

    if (print_csd)
    {
        info += "CSD: ver=" + std::to_string(sd_card_->is_mmc ? sd_card_->csd.csd_ver : sd_card_->csd.csd_ver + 1) +
                ", sector_size=" + std::to_string(sd_card_->csd.sector_size) + ", capacity=" + std::to_string(sd_card_->csd.capacity) +
                ", read_bl_len=" + std::to_string(sd_card_->csd.read_block_len) + "\n";

        if (sd_card_->is_mmc)
        {
            info += "EXT CSD: bus_width=" + std::to_string(1 << sd_card_->log_bus_width) + "\n";
        }
        else if (!sd_card_->is_sdio)
        {
            // make sure card is SD
            info += "SSR: bus_width=" + std::to_string(sd_card_->ssr.cur_bus_width ? 4 : 1) + "\n";
        }
    }

    if (print_scr)
    {
        info += "SCR: sd_spec=" + std::to_string(sd_card_->scr.sd_spec) + ", bus_width=" + std::to_string(sd_card_->scr.bus_width) + "\n";
    }

    return info;
}
