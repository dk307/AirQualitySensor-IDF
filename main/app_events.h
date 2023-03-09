#pragma once

#include <esp_event.h>
#include <stdint.h>

ESP_EVENT_DECLARE_BASE(APP_COMMON_EVENT);

typedef enum
{
    /** App init done*/
    APP_INIT_DONE,

    /** Sensor int done. Data is sensor_id_index */
    SENSOR_VALUE_CHANGE,

    /** Node reboot has been triggered. The associated event data is the time in seconds
     * (type: uint8_t) after which the node will reboot. Note that this time may not be
     * accurate as the events are received asynchronously.*/
    APP_EVENT_REBOOT,
} esp_app_common_event_t;



