#pragma once

#include "ui_screen.h"

class ui_wifi_enroll_screen : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override
    {
        ui_screen::init();
        set_default_screen_color();

        lv_obj_add_event_cb(screen_, event_callback<ui_wifi_enroll_screen, &ui_wifi_enroll_screen::screen_callback>, LV_EVENT_ALL, this);

        auto message_label = create_message_label(screen_, &lv_font_montserrat_24, LV_ALIGN_CENTER, 0, -20, text_color);
        lv_label_set_text_static(message_label, "Use ESPTouch app to set Wifi\ncredentials for the device");

        auto btn_clean = create_btn("Stop Enrollment", event_callback<ui_wifi_enroll_screen, &ui_wifi_enroll_screen::stop_enrollment>);
        lv_obj_align(btn_clean, LV_ALIGN_BOTTOM_MID, 0, -30);
    }

    void show_screen()
    {
        lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }

  private:
    void screen_callback(lv_event_t *e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        if (event_code == LV_EVENT_SCREEN_LOAD_START)
        {
            ui_interface_instance_.start_wifi_enrollment();
        }
        else if (event_code == LV_EVENT_SCREEN_UNLOADED)
        {
            ui_interface_instance_.stop_wifi_enrollment();
        }
    }

    static lv_obj_t *create_message_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                                          lv_color_t color)
    {
        auto *label = lv_label_create(parent);
        lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label, color, LV_PART_MAIN | LV_STATE_DEFAULT);

        return label;
    }

    lv_obj_t *create_btn(const char *label_text, lv_event_cb_t event_cb)
    {
        auto btn_set_baseline = lv_btn_create(screen_);
        lv_obj_add_event_cb(btn_set_baseline, event_cb, LV_EVENT_ALL, this);
        lv_obj_set_height(btn_set_baseline, LV_SIZE_CONTENT);

        auto label = lv_label_create(btn_set_baseline);
        lv_label_set_text_static(label, label_text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(label);

        return btn_set_baseline;
    }

    void stop_enrollment(lv_event_t *e)
    {
        const auto code = lv_event_get_code(e);

        if (code == LV_EVENT_PRESSED)
        {
            ui_interface_instance_.stop_wifi_enrollment();
            inter_screen_interface_.show_home_screen();
        }
    }
};