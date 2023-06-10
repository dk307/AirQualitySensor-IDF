#include "ui_interface.h"
#include "config/config_manager.h"
#include "hardware/display/display.h"
#include "hardware/hardware.h"
#include "hardware/sd_card.h"
#include "homekit/homekit_integration.h"
#include "logging/logging_tags.h"
#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_efuse.h>
#include <esp_flash.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif_types.h>

const sensor_value &ui_interface::get_sensor(sensor_id_index index)
{
    configASSERT(hardware_);
    return hardware_->get_sensor(index);
}

float ui_interface::get_sensor_value(sensor_id_index index)
{
    configASSERT(hardware_);
    return hardware_->get_sensor_value(index);
}

sensor_history::sensor_history_snapshot ui_interface::get_sensor_detail_info(sensor_id_index index)
{
    configASSERT(hardware_);
    return hardware_->get_sensor_detail_info(index);
}

wifi_status ui_interface::get_wifi_status()
{
    configASSERT(wifi_manager_);
    return wifi_manager_->get_wifi_status();
}

bool ui_interface::clean_sps_30()
{
    configASSERT(hardware_);
    return hardware_->clean_sps_30();
}

void ui_interface::start_wifi_enrollment()
{
    configASSERT(hardware_);
    wifi_manager_->start_wifi_enrollment();
}

void ui_interface::stop_wifi_enrollment()
{
    configASSERT(hardware_);
    wifi_manager_->stop_wifi_enrollment();
}

void ui_interface::forget_homekit_pairings()
{
    configASSERT(homekit_integration_);
    homekit_integration_->forget_pairings();
}

void ui_interface::reenable_homekit_pairing()
{
    configASSERT(homekit_integration_);
    homekit_integration_->reenable_pairing();
}

std::string ui_interface::get_up_time()
{
    const auto now = esp_timer_get_time() / (1000 * 1000);
    const auto hour = now / 3600;
    const uint8_t mins = (now % 3600) / 60;
    const uint8_t sec = (now % 3600) % 60;

    if (hour >= 24)
    {
        return esp32::string::sprintf("%lld days %02lld hours %02d mins %02d secs", hour / 24, hour % 24, mins, sec);
    }
    else
    {
        return esp32::string::sprintf("%02lld hours %02d mins %02d secs", hour, mins, sec);
    }
}

std::string ui_interface::get_heap_info_str(uint32_t caps)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, caps);
    return esp32::string::sprintf("%s free out of %s", esp32::string::stringify_size(info.total_free_bytes, 1).c_str(),
                                  esp32::string::stringify_size(info.total_allocated_bytes + info.total_free_bytes, 1).c_str());
}

std::string ui_interface::get_chip_details()
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    std::string_view model;
    switch (chip_info.model)
    {
    case CHIP_ESP32:
        model = "ESP32";
        break;
    case CHIP_ESP32S2:
        model = "ESP32-S2";
        break;
    case CHIP_ESP32S3:
        model = "ESP32-S3";
        break;
    case CHIP_ESP32C3:
        model = "ESP32-C3";
        break;
    default:
        model = "Unknown";
    }

    std::string flash_size_str;
    uint32_t flash_size = 0;
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        ESP_LOGE(OPERATIONS_TAG, "Get flash size failed");
        flash_size_str = "Unknown";
    }
    else
    {
        flash_size_str = esp32::string::stringify_size(flash_size);
    }

    return esp32::string::sprintf("%.*s Rev. %d Cores: %d Flash size: %s", model.length(), model.data(), chip_info.revision,
                                  static_cast<int>(chip_info.cores), flash_size_str.c_str());
}

std::string ui_interface::get_reset_reason_string()
{
    const auto reset_reason = esp_reset_reason();
    std::string_view reset_reason_string;
    switch (reset_reason)
    {
    case ESP_RST_POWERON:
        reset_reason_string = "Power-on reset";
        break;
    case ESP_RST_EXT:
        reset_reason_string = "Reset caused by external pin";
        break;
    case ESP_RST_SW:
        reset_reason_string = "Software reset";
        break;
    case ESP_RST_PANIC:
        reset_reason_string = "Watchdog timer expired or exception occurred";
        break;
    case ESP_RST_INT_WDT:
        reset_reason_string = "Internal Watchdog Timer expired";
        break;
    case ESP_RST_TASK_WDT:
        reset_reason_string = "Task Watchdog Timer expired";
        break;
    case ESP_RST_DEEPSLEEP:
        reset_reason_string = "Wakeup from deep sleep";
        break;
    case ESP_RST_BROWNOUT:
        reset_reason_string = "Brownout reset";
        break;
    case ESP_RST_SDIO:
        reset_reason_string = "Reset caused by SDIO";
        break;
    default:
        reset_reason_string = "Unknown reset reason";
        break;
    }
    return std::string(reset_reason_string);
}

std::string ui_interface::get_version()
{
    const auto desc = esp_app_get_description();
    return esp32::string::sprintf("%s %s %s", desc->date, desc->time, desc->version);
}

std::string ui_interface::get_default_mac_address()
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char mac_address[18];
    sprintf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(mac_address);
}

std::string ui_interface::get_sps30_error_register_status()
{
    configASSERT(hardware_);
    return hardware_->get_sps30_error_register_status();
}

void ui_interface::get_nw_info(ui_interface::information_table_type &table)
{
    esp_netif_ip_info_t ip_info{};

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (netif != NULL)
    {
        // Copy the netif IP info into our variable.
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
        {
            table.push_back({"IP Address", esp32::string::sprintf(IPSTR, IP2STR(&ip_info.ip))});
            table.push_back({"Gateway", esp32::string::sprintf(IPSTR, IP2STR(&ip_info.gw))});
            table.push_back({"Netmask", esp32::string::sprintf(IPSTR, IP2STR(&ip_info.netmask))});
        }

        const char *hostname{nullptr};
        if ((esp_netif_get_hostname(netif, &hostname) == ESP_OK) && hostname)
        {
            table.push_back({"Hostname", hostname});
        }

        esp_netif_dns_info_t dns{};
        if (esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK)
        {
            table.push_back({"DNS", esp32::string::sprintf(IPSTR, IP2STR(&dns.ip.u_addr.ip4))});
        }
    }
}

ui_interface::information_table_type ui_interface::get_information_table(information_type type)
{
    switch (type)
    {
    case information_type::system:
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
        configASSERT(sd_card_);
#endif
        configASSERT(hardware_);
        return {
            {"Version", get_version()},
            {"Chip", get_chip_details()},
            {"Heap", get_heap_info_str(MALLOC_CAP_INTERNAL)},
            {"PsRam", get_heap_info_str(MALLOC_CAP_SPIRAM)},
            {"Uptime", get_up_time()},
            {"Reset Reason", get_reset_reason_string()},
            {"Mac Address", get_default_mac_address()},
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
            {"SD Card", sd_card_->get_info()},
#endif
            {"SPS30 sensor status", hardware_->get_sps30_error_register_status()},
        };

    case information_type::homekit: {
        ui_interface::information_table_type table;

        configASSERT(homekit_integration_);

        const bool paired = homekit_integration_->is_paired();

        table.push_back({"Paired", paired ? "Yes" : "No"});
        if (!paired)
        {
            table.push_back({"Setup Code", homekit_integration_->get_password()});
            table.push_back({"Setup Id", homekit_integration_->get_setup_id()});
        }
        else
        {
            table.push_back({"Connected Clients Count", esp32::string::to_string(homekit_integration_->get_connection_count())});
        }

        return table;
    }

    case information_type::network: {
        ui_interface::information_table_type table;

        wifi_ap_record_t info{};
        const auto result_info = esp_wifi_sta_get_ap_info(&info);
        if (result_info != ESP_OK)
        {
            table.push_back({"Error", esp32::string::sprintf("failed to get info with error:%d", result_info)});
        }
        else
        {
            table.push_back({"Ssid", std::string(reinterpret_cast<char *>(&info.ssid[0]))});
            table.push_back({"RSSI", esp32::string::sprintf("%d db", info.rssi)});
            get_nw_info(table);
        }

        table.push_back({"Mac Address", get_default_mac_address()});

        return table;
    }
    case information_type::config:
        configASSERT(config_);
        return {
            {"Hostname", config_->get_host_name()},
            {"SSID", config_->get_wifi_credentials().get_user_name()},
            {"Web user name", config_->get_web_user_credentials().get_user_name()},
            {"Screen brightness (%)", esp32::string::to_string((100 * config_->get_manual_screen_brightness().value_or(0)) / 256)},
            {"Use Fahrenheit", config_->is_use_fahrenheit() ? "Yes" : "No"},
        };
        break;
    }
    return {};
}
