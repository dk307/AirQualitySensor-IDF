#include "wifi_manager.h"
#include "config/config_manager.h"
#include "logging/logging_tags.h"
#include "util/helper.h"

#include <esp_log.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <memory>
#include <sstream>

wifi_manager wifi_manager::instance;

const int CONNECTED_BIT = BIT0;
const int IP_BIT = BIT1;

void wifi_manager::begin()
{
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_task_.spawn_same("wifi task", 4096, esp32::task::default_priority);
}

bool wifi_manager::connect_saved_wifi()
{
    const auto ssid = config::instance.data.get_wifi_ssid();
    const auto pwd = config::instance.data.get_wifi_password();

    if (!ssid.empty())
    {
        const auto rfc_name = get_rfc_name();
        ESP_LOGI(WIFI_TAG, "Hostname is %s", rfc_name.c_str());
        {
            std::lock_guard<esp32::semaphore> lock(data_mutex_);
            wifi_instance_ = std::make_unique<wifi_sta>(rfc_name, ssid, pwd);
        }
        wifi_instance_->connect_to_ap();
        return wifi_instance_->wait_for_connect(pdMS_TO_TICKS(30000));
    }
    return false;
}

void wifi_manager::wifi_task_ftn()
{
    ESP_LOGI(WIFI_TAG, "Started Wifi Task on Core:%d", xPortGetCoreID());
    do
    {
        connected_to_ap_ = connect_saved_wifi();
        call_change_listeners();

        if (connected_to_ap_)
        {
            wifi_instance_->wait_for_disconnect(portMAX_DELAY);
            connected_to_ap_ = false;
            call_change_listeners();
        }

        vTaskDelay(pdMS_TO_TICKS(30000)); // 30s before retry to connect

    } while (true);
}

std::string wifi_manager::get_rfc_952_host_name(const std::string &name)
{
    const int max_length = 24;
    std::string rfc_952_hostname;
    rfc_952_hostname.reserve(max_length);

    for (auto &&c : name)
    {
        if (isalnum(c) || c == '-')
        {
            rfc_952_hostname += c;
        }
        if (rfc_952_hostname.length() >= max_length)
        {
            break;
        }
    }

    // remove last -
    size_t i = rfc_952_hostname.length() - 1;
    while (rfc_952_hostname[i] == '-' && i > 0)
    {
        i--;
    }

    return rfc_952_hostname.substr(0, i);
}

std::string wifi_manager::get_rfc_name()
{
    auto rfc_name = config::instance.data.get_host_name();
    esp32::trim(rfc_name);

    if (rfc_name.empty())
    {
        uint64_t mac_address = 0LL;
        esp_efuse_mac_get_default((uint8_t *)(&mac_address));

        uint64_t chipId = 0;
        for (auto i = 0; i < 17; i = i + 8)
        {
            chipId |= ((mac_address >> (40 - i)) & 0xff) << i;
        }
        rfc_name = "ESP-" + esp32::format_hex(static_cast<uint32_t>(chipId));
    }

    return get_rfc_952_host_name(rfc_name);
}

bool wifi_manager::is_wifi_connected()
{
    return connected_to_ap_.load();
}

std::string wifi_manager::get_wifi_status()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);

    if (connected_to_ap_)
    {
        if ((wifi_instance_->get_local_ip() != 0))
        {
            return esp32::string::sprintf("Connected to %s with IP %s", wifi_instance_->get_ssid().c_str(),
                                          wifi_instance_->get_local_ip_address().c_str());
        }
        else
        {
            return "Not connected to Wifi";
        }
    }
    else
    {
        return "Not connected to Wifi";
    }
}
