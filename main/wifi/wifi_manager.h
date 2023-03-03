#pragma once

#include "util\change_callback.h"
#include "util\task_wrapper.h"
#include "wifi\wifi_events_notify.h"
#include "wifi\wifi_sta.h"

#include <memory>
#include <string>
#include <atomic>

struct wifi_status
{
    bool connected;
    std::string status;
};

class wifi_manager final : public esp32::change_callback
{
  public:
    void begin();

    wifi_status get_wifi_status();
    static wifi_manager instance;

    static void set_wifi_power_mode(wifi_ps_type_t mode);

  private:
    wifi_manager() : wifi_task_(std::bind(&wifi_manager::wifi_task_ftn, this))
    {
    }
    wifi_events_notify events_notify_;
    std::unique_ptr<wifi_sta> wifi_instance_;
    esp32::task wifi_task_;

    std::atomic_bool connected_to_ap_{false};

    bool connect_saved_wifi();
    void wifi_task_ftn();
    void disconnect();

    std::string_view get_ssid();

    static std::string get_rfc_name();
    static std::string get_rfc_952_host_name(const std::string &name);
};
