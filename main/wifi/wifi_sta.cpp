#include "wifi_sta.h"
#include "logging/logging_tags.h"
#include "util/helper.h"

#include <esp_log.h>
#include <mutex>

/// \brief Copies at min(SourceLen, size) bytes from source to target buffer.
template <typename InputIt, typename SourceLen, typename T, std::size_t size>
void copy_min_to_buffer(InputIt source, SourceLen source_length, T (&target)[size])
{
    auto to_copy = std::min(static_cast<std::size_t>(source_length), size);
    std::copy_n(source, to_copy, target);
}

wifi_sta::wifi_sta(wifi_events_notify &events_notify, const std::string &host_name, const std::string &ssid, const std::string &password)
    : events_notify_(events_notify), host_name_(host_name), ssid_(ssid), password_(password)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta::wifi_event_callback, this, &instance_wifi_event_));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_sta::wifi_event_callback, this, &instance_ip_event_));
}

wifi_sta::~wifi_sta()
{
    esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, instance_ip_event_);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_wifi_event_);
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
}

void wifi_sta::set_host_name(const std::string &name)
{
    host_name_ = name;
}

void wifi_sta::connect_to_ap()
{
    ESP_LOGI(WIFI_TAG, "Connecting to Wifi %s", ssid_.c_str());

    // Prepare to connect to the provided SSID and password
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t config{};
    memset(&config, 0, sizeof(config));
    copy_min_to_buffer(ssid_.begin(), ssid_.length(), config.sta.ssid);
    copy_min_to_buffer(password_.begin(), password_.length(), config.sta.password);

    config.sta.threshold.authmode = password_.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;
    config.sta.bssid_set = false;

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));

    close_if();
    interface_ = esp_netif_create_default_wifi_sta();
    connect();
}

void wifi_sta::connect() const
{
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_sta::wifi_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    auto instance = reinterpret_cast<wifi_sta *>(event_handler_arg);
    instance->wifi_event_callback_impl(event_base, event_id, event_data);
}

void wifi_sta::wifi_event_callback_impl(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            esp_netif_set_hostname(interface_, host_name_.c_str());
        }
        else if (event_id == WIFI_EVENT_STA_CONNECTED)
        {
            events_notify_.set_ap_connected();
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {

            const auto *data = reinterpret_cast<wifi_event_sta_disconnected_t *>(event_data);
            ESP_LOGI(WIFI_EVENT_TAG, "WiFi STA disconnected with reason:%s", get_disconnect_reason_str(data->reason).c_str());
            ip_info_.ip.addr = 0;
            ip_info_.netmask = ip_info_.ip;
            ip_info_.gw = ip_info_.ip;

            events_notify_.set_ap_disconnected();
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP || event_id == IP_EVENT_ETH_GOT_IP)
        {
            ip_info_ = reinterpret_cast<ip_event_got_ip_t *>(event_data)->ip_info;
            ESP_LOGI(WIFI_TAG, "New IP Address : %d.%d.%d.%d", IP2STR(&ip_info_.ip));
            events_notify_.set_ip_connected();
        }
        else if (event_id == IP_EVENT_STA_LOST_IP)
        {
            ip_info_.ip.addr = 0;
            ip_info_.netmask = ip_info_.ip;
            ip_info_.gw = ip_info_.ip;
            events_notify_.set_ip_lost();
        }
    }
}

void wifi_sta::close_if()
{
    if (interface_)
    {
        esp_netif_destroy(interface_);
        interface_ = nullptr;
    }
}

uint32_t wifi_sta::get_local_ip()
{
    return ip_info_.ip.addr;
}

std::string wifi_sta::get_local_ip_address()
{
    std::array<char, 16> str_ip;
    return esp_ip4addr_ntoa(&ip_info_.ip, str_ip.data(), 16);
}

std::string wifi_sta::get_netmask()
{
    std::array<char, 16> str_mask;
    return esp_ip4addr_ntoa(&ip_info_.netmask, str_mask.data(), 16);
}

std::string wifi_sta::get_gateway()
{
    std::array<char, 16> str_gw;
    return esp_ip4addr_ntoa(&ip_info_.gw, str_gw.data(), 16);
}

std::string wifi_sta::get_disconnect_reason_str(uint8_t reason)
{
    switch (reason)
    {
    case WIFI_REASON_AUTH_EXPIRE:
        return "Auth Expired";
    case WIFI_REASON_AUTH_LEAVE:
        return "Auth Leave";
    case WIFI_REASON_ASSOC_EXPIRE:
        return "Association Expired";
    case WIFI_REASON_ASSOC_TOOMANY:
        return "Too Many Associations";
    case WIFI_REASON_NOT_AUTHED:
        return "Not Authenticated";
    case WIFI_REASON_NOT_ASSOCED:
        return "Not Associated";
    case WIFI_REASON_ASSOC_LEAVE:
        return "Association Leave";
    case WIFI_REASON_ASSOC_NOT_AUTHED:
        return "Association not Authenticated";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:
        return "Disassociate Power Cap Bad";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
        return "Disassociate Supported Channel Bad";
    case WIFI_REASON_IE_INVALID:
        return "IE Invalid";
    case WIFI_REASON_MIC_FAILURE:
        return "Mic Failure";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        return "4-Way Handshake Timeout";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
        return "Group Key Update Timeout";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        return "IE In 4-Way Handshake Differs";
    case WIFI_REASON_GROUP_CIPHER_INVALID:
        return "Group Cipher Invalid";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
        return "Pairwise Cipher Invalid";
    case WIFI_REASON_AKMP_INVALID:
        return "AKMP Invalid";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
        return "Unsupported RSN IE version";
    case WIFI_REASON_INVALID_RSN_IE_CAP:
        return "Invalid RSN IE Cap";
    case WIFI_REASON_802_1X_AUTH_FAILED:
        return "802.1x Authentication Failed";
    case WIFI_REASON_CIPHER_SUITE_REJECTED:
        return "Cipher Suite Rejected";
    case WIFI_REASON_BEACON_TIMEOUT:
        return "Beacon Timeout";
    case WIFI_REASON_NO_AP_FOUND:
        return "AP Not Found";
    case WIFI_REASON_AUTH_FAIL:
        return "Authentication Failed";
    case WIFI_REASON_ASSOC_FAIL:
        return "Association Failed";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
        return "Handshake Failed";
    case WIFI_REASON_CONNECTION_FAIL:
        return "Connection Failed";
    case WIFI_REASON_UNSPECIFIED:
    default:
        return esp32::string::to_string(reason);
    }
}
