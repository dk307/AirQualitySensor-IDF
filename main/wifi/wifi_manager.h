#pragma once

#include "app_events.h"
#include "config/config_manager.h"
#include "util/default_event.h"
#include "util/noncopyable.h"
#include "util/singleton.h"
#include "util/task_wrapper.h"
#include "wifi/smart_config_wifi_enroll.h"
#include "wifi/wifi_events_notify.h"
#include "wifi/wifi_sta.h"
#include <atomic>
#include <memory>
#include <string>

struct wifi_status
{
    bool connected;
    std::string status;
};

class wifi_manager final : public esp32::singleton<wifi_manager>
{
  public:
    void begin();

    wifi_status get_wifi_status();
    void start_wifi_enrollment();
    void stop_wifi_enrollment();

    static void set_wifi_power_mode(wifi_ps_type_t mode);

  private:
    wifi_manager(config &config) : config_(config), wifi_task_([this] { wifi_task_ftn(); })
    {
    }
    friend class esp32::singleton<wifi_manager>;

    config &config_;
    wifi_events_notify events_notify_;
    std::unique_ptr<wifi_sta> wifi_instance_;
    esp32::task wifi_task_;

    esp32::default_event_subscriber instance_config_change_event_{APP_COMMON_EVENT, CONFIG_CHANGE,
                                                                  [this](esp_event_base_t, int32_t, void *) { events_notify_.set_config_changed(); }};

    std::atomic_bool connected_to_ap_{false};

    bool connect_saved_wifi();
    void wifi_task_ftn();
    void disconnect();
    void post_wifi_status_changed();

    std::string_view get_ssid();
    std::string get_rfc_name();
    static std::string get_rfc_952_host_name(const std::string &name);
};
