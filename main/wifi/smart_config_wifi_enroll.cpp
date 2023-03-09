#include "smart_config_wifi_enroll.h"
#include "logging/logging_tags.h"
#include "util/exceptions.h"
#include "util/helper.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <smartconfig_ack.h>

smart_config_enroll::smart_config_enroll(wifi_events_notify &events_notify) : events_notify_(events_notify)
{
    instance_sc_event_.subscribe();
    instance_ip_event_.subscribe();
    instance_wifi_event_.subscribe();
}

smart_config_enroll::~smart_config_enroll()
{
    esp_smartconfig_stop();
    instance_sc_event_.unsubscribe();
    instance_ip_event_.unsubscribe();
    instance_wifi_event_.unsubscribe();
    esp_wifi_disconnect();
    esp_wifi_stop();
}

void smart_config_enroll::start()
{
    ESP_LOGI(WIFI_TAG, "Starting Smart Config version:%s", esp_smartconfig_get_version());
    CHECK_THROW_ESP(esp_wifi_set_mode(WIFI_MODE_STA));
    CHECK_THROW_ESP(esp_wifi_start());
}

void smart_config_enroll::wifi_event_callback(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
        ESP_LOGI(WIFI_TAG, "Smart Config Started");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // try reconnect
        esp_wifi_connect();
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGD(WIFI_TAG, "Scan Done");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGD(WIFI_TAG, "Found channel");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(WIFI_TAG, "Got SSID and password");

        const auto evt = reinterpret_cast<smartconfig_event_got_ssid_pswd_t *>(event_data);
        ssid_ = reinterpret_cast<const char *>(evt->ssid);
        password_ = reinterpret_cast<const char *>(evt->password);
        ESP_LOGI(WIFI_TAG, "SSID:%s Psssword:%s", ssid_.c_str(), password_.c_str());

        wifi_config_t wifi_config{};
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set)
        {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));

        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        esp_wifi_connect();
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        ESP_LOGI(WIFI_TAG, "Enrollement finished");
        events_notify_.set_wifi_enrollement_finished();
    }
}