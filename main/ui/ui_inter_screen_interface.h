#pragma once

#include "hardware/sensors/sensor_id.h"
#include <string>

class ui_inter_screen_interface
{
  public:
    virtual bool is_night_theme_enabled() = 0;
    virtual void show_home_screen() = 0;
    virtual void show_setting_screen() = 0;
    virtual void show_sensor_detail_screen(sensor_id_index index) = 0;
    virtual void show_launcher_screen() = 0;
    virtual void show_hardware_info_screen() = 0;
    virtual void show_homekit_screen() = 0;
    virtual void show_wifi_enroll_screen() = 0;
    virtual void show_top_level_message(const std::string &message, uint32_t period) = 0;
};