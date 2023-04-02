#pragma once

#include "util/default_event.h"
#include "util/noncopyable.h"
#include "wifi_events_notify.h"
#include <esp_netif_types.h>
#include <esp_smartconfig.h>
#include <esp_wifi_types.h>
#include <string>

class smart_config_enroll final : esp32::noncopyable
{
  public:
    smart_config_enroll(wifi_events_notify &events_notify_);
    ~smart_config_enroll();

    void start();

    const std::string &get_ssid() const
    {
        return ssid_;
    }
    const std::string &get_password() const
    {
        return password_;
    }

  private:
    wifi_events_notify &events_notify_;
    const esp32::default_event_subscriber::callback_t event_callback = [this](esp_event_base_t event_base, int32_t event_id, void *event_data) {
        wifi_event_callback(event_base, event_id, event_data);
    };
    esp32::default_event_subscriber instance_wifi_event_{IP_EVENT, ESP_EVENT_ANY_ID, event_callback};
    esp32::default_event_subscriber instance_ip_event_{WIFI_EVENT, ESP_EVENT_ANY_ID, event_callback};
    esp32::default_event_subscriber instance_sc_event_{SC_EVENT, ESP_EVENT_ANY_ID, event_callback};

    std::string ssid_;
    std::string password_;

    void wifi_event_callback(esp_event_base_t event_base, int32_t event_id, void *event_data);
};