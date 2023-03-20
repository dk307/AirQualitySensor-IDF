
#pragma once

#include "config/credentials.h"
#include "util/default_event.h"
#include "util/noncopyable.h"
#include "util/semaphore_lockable.h"
#include "wifi/wifi_events_notify.h"
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <string>

/// Wifi sta class
class wifi_sta final : esp32::noncopyable
{
  public:
    wifi_sta(wifi_events_notify &events_notify_, const std::string &host_name, const credentials &password);
    ~wifi_sta();

    /// Initiates the connection to the AP.
    void connect_to_ap();

    const credentials &get_credentials() const
    {
        return credentials_;
    }

  private:
    wifi_events_notify &events_notify_;
    const std::string host_name_;
    const credentials credentials_;

    const esp32::default_event_subscriber::callback_t event_callback = [this](esp_event_base_t event_base, int32_t event_id, void *event_data) {
        wifi_event_callback(event_base, event_id, event_data);
    };

    esp32::default_event_subscriber instance_wifi_event_{WIFI_EVENT, ESP_EVENT_ANY_ID, event_callback};
    esp32::default_event_subscriber instance_ip_event_{IP_EVENT, ESP_EVENT_ANY_ID, event_callback};

    void connect() const;
    void wifi_event_callback(esp_event_base_t event_base, int32_t event_id, void *event_data);
    static std::string get_disconnect_reason_str(uint8_t reason);
};
