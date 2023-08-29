#include "wifi_sta.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
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

wifi_sta::wifi_sta(wifi_events_notify &events_notify, const std::string &host_name, const credentials &credentials)
    : events_notify_(events_notify), host_name_(host_name), credentials_(credentials)
{
    instance_wifi_event_.subscribe();
    instance_ip_event_.subscribe();
}

wifi_sta::~wifi_sta()
{
    instance_wifi_event_.unsubscribe();
    instance_ip_event_.unsubscribe();
    esp_wifi_disconnect();
    esp_wifi_stop();
}

void wifi_sta::connect_to_ap()
{
    ESP_LOGI(WIFI_TAG, "Connecting to Wifi %s", credentials_.get_user_name().c_str());

    // Prepare to connect to the provided SSID and password
    CHECK_THROW_ESP(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t config{};
    copy_min_to_buffer(credentials_.get_user_name().begin(), credentials_.get_user_name().length(), config.sta.ssid);
    copy_min_to_buffer(credentials_.get_password().begin(), credentials_.get_password().length(), config.sta.password);
    config.sta.threshold.authmode = credentials_.get_password().empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    config.sta.pmf_cfg.capable = true;
    config.sta.pmf_cfg.required = false;

    CHECK_THROW_ESP(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    CHECK_THROW_ESP(esp_wifi_set_config(WIFI_IF_STA, &config));

    connect();
}

void wifi_sta::connect() const
{
    CHECK_THROW_ESP(esp_wifi_start());
    CHECK_THROW_ESP(esp_wifi_set_dynamic_cs(true));
    CHECK_THROW_ESP(esp_wifi_connect());
}

void wifi_sta::wifi_event_callback(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            const auto err = esp_netif_set_hostname(netif, host_name_.c_str());
            if (err != ESP_OK)
            {
                ESP_LOGW(WIFI_EVENT_TAG, "Failed to set hostname with error:%s", esp_err_to_name(err));
            }
        }
        else if (event_id == WIFI_EVENT_STA_CONNECTED)
        {
            events_notify_.set_ap_connected();
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            const auto *data = reinterpret_cast<wifi_event_sta_disconnected_t *>(event_data);
            ESP_LOGI(WIFI_EVENT_TAG, "WiFi STA disconnected with reason:%s", get_disconnect_reason_str(data->reason).c_str());
            events_notify_.set_ap_disconnected();
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP || event_id == IP_EVENT_ETH_GOT_IP)
        {
            auto ip_info = reinterpret_cast<ip_event_got_ip_t *>(event_data)->ip_info;
            ESP_LOGI(WIFI_TAG, "New IP Address : %d.%d.%d.%d", IP2STR(&ip_info.ip));
            events_notify_.set_ip_connected();
        }
        else if (event_id == IP_EVENT_STA_LOST_IP)
        {
            events_notify_.set_ip_lost();
        }
    }
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
