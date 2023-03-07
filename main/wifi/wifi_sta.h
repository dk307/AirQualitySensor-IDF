
#pragma once

#include "util/noncopyable.h"
#include "util/semaphore_lockable.h"
#include "wifi/wifi_events_notify.h"

#include "config/credentials.h"
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

    /// Sets the hostname
    /// \param name The name
    void set_host_name(const std::string &name);

    /// Initiates the connection to the AP.
    void connect_to_ap();

    const credentials &get_credentials() const
    {
        return credentials_;
    }

  private:
    wifi_events_notify &events_notify_;
    std::string host_name_;
    const credentials credentials_;

    esp_event_handler_instance_t instance_wifi_event_{};
    esp_event_handler_instance_t instance_ip_event_{};
    esp_netif_ip_info_t ip_info_{};

    void connect() const;
    void close_if();
    static void wifi_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void wifi_event_callback_impl(esp_event_base_t event_base, int32_t event_id, void *event_data);
    static std::string get_disconnect_reason_str(uint8_t reason);
};
