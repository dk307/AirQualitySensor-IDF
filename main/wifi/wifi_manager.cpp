#include "wifi_manager.h"
#include "config/config_manager.h"
#include "logging/logging_tags.h"
#include "operations/operations.h"
#include "util/cores.h"
#include "util/exceptions.h"
#include "util/helper.h"

#include <esp_log.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <memory>

wifi_manager wifi_manager::instance;

void wifi_manager::begin()
{
    CHECK_THROW_INIT(esp_netif_init());

    config::instance.add_callback([this] { events_notify_.set_config_changed(); });
    wifi_task_.spawn_pinned("wifi task", 4096, esp32::task::default_priority, esp32::wifi_core);
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
            wifi_instance_ = std::make_unique<wifi_sta>(events_notify_, rfc_name, ssid, pwd);
        }
        events_notify_.clear_connection_bits();
        wifi_instance_->connect_to_ap();
        return events_notify_.wait_for_connect(pdMS_TO_TICKS(30000));
    }
    return false;
}

std::string wifi_manager::get_ssid()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return (wifi_instance_) ? wifi_instance_->get_ssid() : std::string();
}

void wifi_manager::wifi_task_ftn()
{
    ESP_LOGI(WIFI_TAG, "Started Wifi Task on Core:%d", xPortGetCoreID());
    try
    {
        do
        {
            // not connected or ssid changed
            if (!connected_to_ap_ || (get_ssid() != config::instance.data.get_wifi_ssid()))
            {
                disconnect();
                connected_to_ap_ = connect_saved_wifi();
                call_change_listeners();
            }

            const auto bits_set = events_notify_.wait_for_any_event(connected_to_ap_ ? portMAX_DELAY : pdMS_TO_TICKS(15000));

            if (operations::instance.get_reset_pending())
            {
                // dont do anything if restarting
                ESP_LOGI(WIFI_TAG, "Reset pending, wifi disconnect ignore");
                break;
            }

            if (bits_set & (wifi_events_notify::DISCONNECTED_BIT | wifi_events_notify::LOSTIP_BIT))
            {
                ESP_LOGI(WIFI_TAG, "Wifi disconnected");
                connected_to_ap_ = false;
                call_change_listeners();
            }

            if (bits_set & wifi_events_notify::CONFIG_CHANGED)
            {
                ESP_LOGD(WIFI_TAG, "Config Changed");
            }

        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGI(OPERATIONS_TAG, "Wifi Task Failure:%s", ex.what());
        // Long wait is intentional to debug wifi issues
        vTaskDelay(pdMS_TO_TICKS(60 * 1000));
        operations::instance.reboot();
    }

    vTaskDelete(NULL);
}

void wifi_manager::disconnect()
{
    connected_to_ap_ = false;
    bool changed = false;
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        changed = wifi_instance_ != nullptr;
        wifi_instance_.reset();
    }

    if (changed)
    {
        call_change_listeners();
    }
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

wifi_status wifi_manager::get_wifi_status()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    if (connected_to_ap_ && wifi_instance_)
    {
        if ((wifi_instance_->get_local_ip() != 0))
        {
            return {connected_to_ap_, esp32::string::sprintf("Connected to %s with IP %s", wifi_instance_->get_ssid().c_str(),
                                                             wifi_instance_->get_local_ip_address().c_str())};
        }
        else
        {
            return {connected_to_ap_, "Not connected to Wifi"};
        }
    }
    else
    {
        return {connected_to_ap_, "Not connected to Wifi"};
    }
}

void wifi_manager::set_wifi_power_mode(wifi_ps_type_t mode)
{
    CHECK_THROW_WIFI(esp_wifi_set_ps(mode));
}
