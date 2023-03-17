
#pragma once

#include "util/noncopyable.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

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

    void clear_enrollment_bits()
    {
        xEventGroupClearBits(wifi_event_group_, WIFI_ENROLL_START | WIFI_ENROLL_DONE | WIFI_ENROLL_CANCEL);
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

    void set_start_wifi_enrollement()
    {
        xEventGroupSetBits(wifi_event_group_, WIFI_ENROLL_START);
    }

    void set_wifi_enrollement_finished()
    {
        xEventGroupSetBits(wifi_event_group_, WIFI_ENROLL_DONE);
    }

    void set_wifi_enrollement_cencelled()
    {
        xEventGroupSetBits(wifi_event_group_, WIFI_ENROLL_CANCEL);
    }

    bool wait_for_connect(TickType_t time)
    {
        const TickType_t final_time = xTaskGetTickCount() + time;
        constexpr auto connected_bits = CONNECTED_BIT | GOTIP_BIT;
        constexpr auto disconnected_bits = DISCONNECTED_BIT | LOSTIP_BIT | WIFI_ENROLL_START;

        EventBits_t set_bits = 0;
        int64_t wait = time;
        do
        {
            set_bits |= xEventGroupWaitBits(wifi_event_group_, connected_bits | disconnected_bits,
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

    EventBits_t wait_for_events(TickType_t time)
    {
        return xEventGroupWaitBits(wifi_event_group_, DISCONNECTED_BIT | LOSTIP_BIT | CONFIG_CHANGED | WIFI_ENROLL_START,
                                   pdTRUE,  // clear before return
                                   pdFALSE, // wait for any
                                   time);
    }

    EventBits_t wait_for_enrollement_complete(TickType_t time)
    {
        return xEventGroupWaitBits(wifi_event_group_, WIFI_ENROLL_DONE | WIFI_ENROLL_CANCEL,
                                   pdTRUE,  // clear before return
                                   pdFALSE, // wait for any
                                   time);
    }

    constexpr static uint32_t CONNECTED_BIT = BIT0;
    constexpr static uint32_t DISCONNECTED_BIT = BIT1;
    constexpr static uint32_t GOTIP_BIT = BIT2;
    constexpr static uint32_t LOSTIP_BIT = BIT3;
    constexpr static uint32_t CONFIG_CHANGED = BIT4;
    constexpr static uint32_t WIFI_ENROLL_START = BIT5;
    constexpr static uint32_t WIFI_ENROLL_DONE = BIT6;
    constexpr static uint32_t WIFI_ENROLL_CANCEL = BIT7;

  private:
    EventGroupHandle_t wifi_event_group_;
    StaticEventGroup_t event_group_buffer_;
};
