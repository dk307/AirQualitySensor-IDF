#pragma once

#include "sensor/sensor_id.h"

class ui_inter_screen_interface
{
  public:
    virtual void show_home_screen() = 0;
    virtual void show_setting_screen() = 0;
    virtual void show_sensor_detail_screen(sensor_id_index index) = 0;
    virtual void show_launcher_screen() = 0;
    virtual void show_hardware_info_screen() = 0;
    virtual void show_homekit_screen() = 0;
    virtual void show_wifi_enroll_screen() = 0;
    virtual void show_top_level_message(const std::string &message, uint32_t period) = 0;
};