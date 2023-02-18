#pragma once

#include "wifi_sta.h"
#include "util\change_callback.h"
#include "util\task_wrapper.h"

#include <string>
#include <memory>

class wifi_manager : public esp32::change_callback
{
public:
    void begin();

    bool is_wifi_connected();
    std::string get_wifi_status();
    static wifi_manager instance;

private:
    wifi_manager() : wifi_task(std::bind(&wifi_manager::wifi_task_ftn, this))
    {
    }
    mutable esp32::semaphore data_mutex;
    std::unique_ptr<wifi_sta> wifi_instance;
    esp32::task wifi_task;

    std::atomic_bool connected_to_ap{false};

    bool connect_saved_wifi();
    void wifi_task_ftn();

    static std::string get_rfc_name();
    static std::string get_rfc_952_host_name(const std::string &name);
};
