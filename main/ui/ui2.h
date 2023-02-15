#pragma once

#include <lvgl.h>

#include "sensor\sensor_id.h"
#include "sensor\sensor.h"
#include "ui_interface.h"

#include "ui_boot_screen.h"
#include "ui_main_screen.h"
#include "ui_sensor_detail_screen.h"
#include "ui_information_screen.h"
#include "ui_inter_screen_interface.h"
#include "ui_launcher_screen.h"
#include "ui_hardware_info_screen.h"

class ui : public ui_inter_screen_interface
{
public:
    ui(ui_interface &ui_interface_) : ui_interface_instance(ui_interface_)
    {
    }
    void init();
    void update_boot_message(const std::string &message);
    void show_top_level_message(const std::string &message, uint32_t period = top_message_timer_period);
    void set_sensor_value(sensor_id_index id, const std::optional<sensor_value::value_type> &value);
    void set_main_screen();
    void wifi_changed();

    // ui_inter_screen_interface
    void show_home_screen() override
    {
        main_screen.show_screen();
    }

    void show_setting_screen() override
    {
        settings_screen.show_screen();
    }
    void show_sensor_detail_screen(sensor_id_index index) override
    {
        sensor_detail_screen.show_screen(index);
    }

    void show_launcher_screen() override
    {
        lv_obj_add_flag(top_message_panel, LV_OBJ_FLAG_HIDDEN);
        launcher_screen.show_screen();
    }

    void show_hardware_info_screen() override
    {
        lv_obj_add_flag(top_message_panel, LV_OBJ_FLAG_HIDDEN);
        hardware_info_screen.show_screen();
    }

private:
    ui_interface &ui_interface_instance;

    static const uint32_t top_message_timer_period = 10000;

    // top sys layer
    lv_obj_t *no_wifi_image;
    lv_style_t no_wifi_image_style;
    lv_anim_timeline_t *no_wifi_image_animation_timeline;

    lv_obj_t *top_message_panel;
    lv_obj_t *top_message_label;
    lv_timer_t *top_message_timer;

    ui_common_fonts common_fonts{};

    ui_boot_screen boot_screen{ui_interface_instance, *this, &common_fonts};
    ui_main_screen main_screen{ui_interface_instance, *this, &common_fonts};
    ui_sensor_detail_screen sensor_detail_screen{ui_interface_instance, *this, &common_fonts};
    ui_information_screen settings_screen{ui_interface_instance, *this, &common_fonts};
    ui_launcher_screen launcher_screen{ui_interface_instance, *this, &common_fonts};
    ui_hardware_info_screen hardware_info_screen{ui_interface_instance, *this, &common_fonts};

    void load_from_sd_card();
    void init_no_wifi_image();
    void init_top_message();
    static void no_wifi_img_animation_cb(void *var, int32_t v);
    static void top_message_timer_cb(lv_timer_t *e);
    static lv_font_t * lv_font_from_sd_card(const char *path);
};
