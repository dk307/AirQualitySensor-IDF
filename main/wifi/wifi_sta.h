
#pragma once

#include "util/semaphore_lockable.h"
#include <array>
#include <atomic>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos\event_groups.h>
#include <string>

/// Wifi sta class
class wifi_sta
{
  public:
    wifi_sta(bool auto_connect_to_ap, const std::string &host_name, const std::string &ssid, const std::string &password);

    wifi_sta(const wifi_sta &) = delete;
    wifi_sta(wifi_sta &&) = delete;
    wifi_sta &operator=(const wifi_sta &) = delete;
    wifi_sta &operator=(wifi_sta &&) = delete;

    ~wifi_sta();

    /// Sets the hostname
    /// \param name The name
    void set_host_name(const std::string &name);

    /// Initiates the connection to the AP.
    void connect_to_ap();

    /// Returns a value indicating of currently connected to the access point.
    /// \return
    [[nodiscard]] bool is_connected_to_ap() const;

    [[nodiscard]] static std::string get_mac_address();

    [[nodiscard]] static bool get_local_mac_address(std::array<uint8_t, 6> &m);

    [[nodiscard]] uint32_t get_local_ip();

    [[nodiscard]] std::string get_local_ip_address();

    [[nodiscard]] std::string get_netmask();

    [[nodiscard]] std::string get_gateway();

    const std::string &get_ssid() const
    {
        return ssid_;
    }

    bool wait_for_connect(TickType_t time);
    bool wait_for_disconnect(TickType_t time);

  private:
    void connect() const;
    void close_if();

    static void wifi_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    void wifi_event_callback_impl(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static std::string get_disconnect_reason_str(uint8_t reason);

    const bool auto_connect_to_ap_;
    std::string host_name_;
    const std::string ssid_;
    const std::string password_;

    EventGroupHandle_t wifi_event_group_;

    esp_netif_t *interface_{nullptr};

    esp_event_handler_instance_t instance_wifi_event_{};
    esp_event_handler_instance_t instance_ip_event_{};
    esp_netif_ip_info_t ip_info_{};
};
