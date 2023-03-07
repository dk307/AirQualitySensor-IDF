#pragma once

#include "util\noncopyable.h"
#include "wifi_events_notify.h"
#include <esp_event.h>
#include <string>

class smart_config_enroll : esp32::noncopyable
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
    esp_event_handler_instance_t instance_wifi_event_{};
    esp_event_handler_instance_t instance_ip_event_{};
    esp_event_handler_instance_t instance_sc_event_{};

    std::string ssid_;
    std::string password_;

    static void wifi_event_callback(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void wifi_event_callback_impl(esp_event_base_t event_base, int32_t event_id, void *event_data);
};