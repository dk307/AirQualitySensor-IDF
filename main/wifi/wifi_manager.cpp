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

void wifi_manager::begin()
{
    CHECK_THROW_ESP(esp_netif_init());

    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    CHECK_THROW_ESP(esp_wifi_init(&init));
    CHECK_THROW_ESP(esp_wifi_set_ps(WIFI_PS_NONE));

    esp_netif_create_default_wifi_sta();

    instance_config_change_event_.subscribe();
    wifi_task_.spawn_pinned("wifi_task", 4 * 1024, esp32::task::default_priority, esp32::wifi_core);
}

bool wifi_manager::connect_saved_wifi()
{
    const auto wifi_credentials = config_.get_wifi_credentials();

    if (!wifi_credentials.empty())
    {
        ESP_LOGD(WIFI_TAG, "Connecting to saved Wifi connection:%s", wifi_credentials.get_user_name().c_str());
        const auto rfc_name = get_rfc_name();
        ESP_LOGI(WIFI_TAG, "Hostname is %s", rfc_name.c_str());

        wifi_instance_ = std::make_unique<wifi_sta>(events_notify_, rfc_name, wifi_credentials);
        events_notify_.clear_connection_bits();

        ESP_LOGD(WIFI_TAG, "Waiting for Wifi connection");
        wifi_instance_->connect_to_ap();
        return events_notify_.wait_for_connect(pdMS_TO_TICKS(30000));
    }
    else
    {
        ESP_LOGW(WIFI_TAG, "No wifi connection configured");
        return false;
    }
}

void wifi_manager::start_wifi_enrollment()
{
    ESP_LOGI(WIFI_TAG, "Signalled starting Wifi enrollement");
    events_notify_.set_start_wifi_enrollement();
}

void wifi_manager::stop_wifi_enrollment()
{
    ESP_LOGI(WIFI_TAG, "Signalled cancel Wifi enrollement");
    events_notify_.set_wifi_enrollement_cencelled();
}

std::string_view wifi_manager::get_ssid()
{
    return (wifi_instance_) ? wifi_instance_->get_credentials().get_user_name() : std::string_view();
}

void wifi_manager::wifi_task_ftn()
{
    ESP_LOGI(WIFI_TAG, "Started Wifi Task on Core:%d", xPortGetCoreID());
    connected_to_ap_ = false;
    try
    {
        do
        {
            // not connected or ssid changed
            if (!connected_to_ap_ || (get_ssid() != config_.get_wifi_credentials().get_user_name()))
            {
                disconnect();
                connected_to_ap_ = connect_saved_wifi();
                if (connected_to_ap_)
                {
                    ESP_LOGI(WIFI_TAG, "Connected to Wifi");
                }
                else
                {
                    events_notify_.clear_connection_bits();
                    ESP_LOGW(WIFI_TAG, "Failed to connect to Wifi");
                }
                post_wifi_status_changed();
            }

            const auto bits_set = events_notify_.wait_for_events(connected_to_ap_ ? portMAX_DELAY : pdMS_TO_TICKS(8000));

            if (operations::instance.get_reset_pending())
            {
                // dont do anything if restarting
                ESP_LOGI(WIFI_TAG, "Reset pending, wifi disconnect is ignored");
                break;
            }

            if (bits_set & wifi_events_notify::WIFI_ENROLL_START)
            {
                ESP_LOGI(WIFI_TAG, "Wifi Enrollment start");
                disconnect();

                events_notify_.clear_enrollment_bits();

                auto wifi_enroll_instance = std::make_unique<smart_config_enroll>(events_notify_);
                wifi_enroll_instance->start();
                const auto bits = events_notify_.wait_for_enrollement_complete(portMAX_DELAY);

                events_notify_.clear_enrollment_bits();

                if (bits & wifi_events_notify::WIFI_ENROLL_CANCEL)
                {
                    ESP_LOGI(WIFI_TAG, "Wifi enrollment was cancelled");
                }
                else
                {
                    ESP_LOGI(WIFI_TAG, "Wifi enrollment done");

                    const auto ssid = wifi_enroll_instance->get_ssid();
                    if (!ssid.empty())
                    {
                        ESP_LOGI(WIFI_TAG, "Updating wifi ssid/password");
                        config_.set_wifi_credentials(credentials(ssid, wifi_enroll_instance->get_password()));
                        config_.save();
                    }
                    else
                    {
                        ESP_LOGW(WIFI_TAG, "Empty ssid after enrollement");
                    }
                }
            }

            if (bits_set & (wifi_events_notify::DISCONNECTED_BIT | wifi_events_notify::LOSTIP_BIT))
            {
                ESP_LOGI(WIFI_TAG, "Wifi disconnected");
                connected_to_ap_ = false;
                post_wifi_status_changed();
            }

            if (bits_set & wifi_events_notify::CONFIG_CHANGED)
            {
                ESP_LOGI(WIFI_TAG, "Config Changed");
            }
        } while (true);
    }
    catch (const std::exception &ex)
    {
        ESP_LOGE(WIFI_TAG, "Wifi Task Failure:%s", ex.what());
        throw;
    }

    vTaskDelete(NULL);
}

void wifi_manager::disconnect()
{
    connected_to_ap_ = false;
    const auto changed = wifi_instance_ != nullptr;
    wifi_instance_.reset();

    if (changed)
    {
        post_wifi_status_changed();
    }
}

std::string wifi_manager::get_rfc_952_host_name(const std::string &name)
{
    constexpr int max_length = 24;
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
    auto rfc_name = config_.get_host_name();
    esp32::trim(rfc_name);

    if (rfc_name.empty())
    {
        uint64_t mac_address = 0LL;
        esp_efuse_mac_get_default(reinterpret_cast<uint8_t *>(&mac_address));

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
    wifi_status status{};

    if (connected_to_ap_)
    {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

        if (netif != NULL)
        {
            esp_netif_ip_info_t ip_info{};
            if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
            {
                if (ip_info.ip.addr != 0)
                {
                    auto ssid = get_ssid();
                    return {true, esp32::string::sprintf("Connected to %.*s", ssid.size(), ssid.data())};
                }
            }
        }
    }
    return {false, "Not connected to Wifi"};
}

void wifi_manager::set_wifi_power_mode(wifi_ps_type_t mode)
{
    CHECK_THROW_ESP(esp_wifi_set_ps(mode));
}

void wifi_manager::post_wifi_status_changed()
{
    CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, WIFI_STATUS_CHANGED));
}