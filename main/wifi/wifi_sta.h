
#pragma once

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
    wifi_sta(wifi_events_notify &events_notify_, const std::string &host_name, const std::string &ssid, const std::string &password);
    ~wifi_sta();

    /// Sets the hostname
    /// \param name The name
    void set_host_name(const std::string &name);

    /// Initiates the connection to the AP.
    void connect_to_ap();

    [[nodiscard]] uint32_t get_local_ip();

    [[nodiscard]] std::string get_local_ip_address();

    [[nodiscard]] std::string get_netmask();

    [[nodiscard]] std::string get_gateway();

    const std::string &get_ssid() const
    {
        return ssid_;
    }

  private:
    void connect() const;
    void close_if();

    static void wifi_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    void wifi_event_callback_impl(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static std::string get_disconnect_reason_str(uint8_t reason);

    wifi_events_notify &events_notify_;
    std::string host_name_;
    const std::string ssid_;
    const std::string password_;

    esp_netif_t *interface_{nullptr};

    esp_event_handler_instance_t instance_wifi_event_{};
    esp_event_handler_instance_t instance_ip_event_{};
    esp_netif_ip_info_t ip_info_{};
};
