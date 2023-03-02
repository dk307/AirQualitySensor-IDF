
#pragma once

#include "util/noncopyable.h"

#include <freertos\event_groups.h>

class wifi_events_notify final : esp32::noncopyable
{
  public:
    wifi_events_notify()
    {
        wifi_event_group_ = xEventGroupCreateStatic(&event_group_buffer_);
        configASSERT(wifi_event_group_);
    }

    ~wifi_events_notify()
    {
        vEventGroupDelete(wifi_event_group_);
    }

    void clear_connection_bits()
    {
        xEventGroupClearBits(wifi_event_group_, CONNECTED_BIT | GOTIP_BIT | DISCONNECTED_BIT | LOSTIP_BIT);
    }

    void set_ap_connected()
    {
        xEventGroupSetBits(wifi_event_group_, CONNECTED_BIT);
    }

    void set_config_changed()
    {
        xEventGroupSetBits(wifi_event_group_, CONFIG_CHANGED);
    }

    void set_ip_connected()
    {
        xEventGroupSetBits(wifi_event_group_, GOTIP_BIT);
    }

    void set_ap_disconnected()
    {
        xEventGroupSetBits(wifi_event_group_, DISCONNECTED_BIT);
    }

    void set_ip_lost()
    {
        xEventGroupSetBits(wifi_event_group_, LOSTIP_BIT);
    }

    bool wait_for_connect(TickType_t time)
    {
        const TickType_t final_time = xTaskGetTickCount() + time;
        const auto connected_bits = CONNECTED_BIT | GOTIP_BIT;
        const auto disconnected_bits = DISCONNECTED_BIT | LOSTIP_BIT;

        EventBits_t set_bits = 0;
        int64_t wait = time;
        do
        {
            set_bits |= xEventGroupWaitBits(wifi_event_group_, CONNECTED_BIT | DISCONNECTED_BIT | GOTIP_BIT | LOSTIP_BIT,
                                            pdTRUE,  // clear before return
                                            pdFALSE, // wait for any
                                            wait);

            if (set_bits & disconnected_bits) // any bit is set
            {
                xEventGroupSetBits(wifi_event_group_, set_bits & disconnected_bits);
                return false;
            }

            if ((set_bits & connected_bits) == connected_bits)
            {
                return true;
            }
            wait = final_time - xTaskGetTickCount();
        } while (wait);

        return false;
    }

    EventBits_t wait_for_any_event(TickType_t time)
    {
        return xEventGroupWaitBits(wifi_event_group_, CONNECTED_BIT | DISCONNECTED_BIT | GOTIP_BIT | LOSTIP_BIT | CONFIG_CHANGED,
                                   pdTRUE,  // clear before return
                                   pdFALSE, // wait for any
                                   time);
    }

    const static auto CONNECTED_BIT = BIT0;
    const static auto DISCONNECTED_BIT = BIT1;
    const static auto GOTIP_BIT = BIT2;
    const static auto LOSTIP_BIT = BIT3;
    const static auto CONFIG_CHANGED = BIT4;

  private:
    EventGroupHandle_t wifi_event_group_;
    StaticEventGroup_t  event_group_buffer_;;
};
