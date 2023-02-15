#pragma once

#include <lvgl.h>
#include <esp_log.h>

#include "ui_interface.h"
#include "ui_inter_screen_interface.h"
#include "logging/logging_tags.h"

struct ui_common_fonts
{
    // fonts loaded from sd card - bpp4
    lv_font_t *font_big_panel;                     // 0x20,0,1,2,3,4,5,6,7,8,9,-
    lv_font_t *font_temp_hum;                      // 0x20,0,1,2,3,4,5,6,7,8,9,F,µ,g,/,m,³,°,F,⁒,p,-
    lv_font_t *font_montserrat_regular_numbers_40; // 0x20,0,1,2,3,4,5,6,7,8,9,-
    lv_font_t *font_montserrat_medium_48;          // 0x20-0x7F,0,1,2,3,4,5,6,7,8,9,F,µ,g,/,m,³,°,F,⁒,p,-
    lv_font_t *font_montserrat_medium_units_18;    // 0x20,F,µ,g,/,m,³,°,F,⁒,p,-
    lv_font_t *font_montserrat_medium_14;          // 0x20-0x7F,0,1,2,3,4,5,6,7,8,9,F,µ,g,/,m,³,°,F,⁒,p,-
};

class ui_screen
{
public:
    ui_screen(ui_interface &ui_interface_instance_, ui_inter_screen_interface &ui_inter_screen_interface_, const ui_common_fonts *fonts_)
        : ui_interface_instance(ui_interface_instance_), inter_screen_interface(ui_inter_screen_interface_), fonts(fonts_)
    {
    }

    virtual void init()
    {
        screen = lv_obj_create(NULL);
    }

    bool is_active()
    {
        return lv_scr_act() == screen;
    }

protected:
    const static int screen_width = 480;
    const static int screen_height = 320;
    const lv_color_t off_black_color = lv_color_hex(0x1E1E1E);
    const lv_color_t text_color = lv_color_hex(0xFFFAFA);

    ui_interface &ui_interface_instance;
    ui_inter_screen_interface &inter_screen_interface;
    const ui_common_fonts *fonts;
    lv_obj_t *screen{nullptr};

    static void set_padding_zero(lv_obj_t *obj)
    {
        lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    template <class T, void (T::*ftn)(lv_event_t *)>
    static void event_callback(lv_event_t *e)
    {
        auto p_this = reinterpret_cast<T *>(lv_event_get_user_data(e));
        (p_this->*ftn)(e);
    }

    template <class T, void (T::*ftn)(lv_timer_t *)>
    static void timer_callback(lv_timer_t *e)
    {
        auto p_this = reinterpret_cast<T *>(e->user_data);
        (p_this->*ftn)(e);
    }

    // do not call this in loop, only for first time init
    static struct _lv_event_dsc_t *add_event_callback(lv_obj_t *obj, const std::function<void(lv_event_t *)> &ftn, lv_event_code_t filter = LV_EVENT_ALL)
    {
        auto param = new std::function<void(lv_event_t *)>(ftn); // never freed
        return lv_obj_add_event_cb(obj, event_callback_ftn, filter, param);
    }

    void create_close_button_to_main_screen(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)
    {
        lv_obj_t *close_button = lv_btn_create(parent);
        lv_obj_add_flag(close_button, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(close_button, lv_color_white(), LV_STATE_CHECKED);
        lv_obj_set_style_pad_all(close_button, 15, 0);
        lv_obj_set_style_radius(close_button, LV_RADIUS_CIRCLE, 0);

        lv_obj_set_style_shadow_width(close_button, 0, 0);
        lv_obj_set_style_bg_img_src(close_button, LV_SYMBOL_HOME, 0);

        lv_obj_set_size(close_button, LV_DPX(48), LV_DPX(48));
        lv_obj_align(close_button, align, x_ofs, y_ofs);

        lv_obj_add_event_cb(close_button, event_callback<ui_screen, &ui_screen::close_button_callback>, LV_EVENT_SHORT_CLICKED, this);
    }

    void set_default_screen()
    {
        lv_obj_set_style_bg_grad_dir(screen, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x0C0D0C), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(screen, lv_color_hex(0x111210), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

private:
    static void event_callback_ftn(lv_event_t *e)
    {
        auto p_ftn = reinterpret_cast<std::function<void(lv_event_t * e)> *>(lv_event_get_user_data(e));
        (*p_ftn)(e);
    }

    void close_button_callback(lv_event_t *e)
    {
        if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
        {
            inter_screen_interface.show_home_screen();
        }
    }
};
