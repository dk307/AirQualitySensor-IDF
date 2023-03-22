#pragma once

#include "ui_screen.h"

class ui_information_screen : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override
    {
        ui_screen::init();
        set_default_screen_color();

        tab_view_ = lv_tabview_create(screen_, LV_DIR_LEFT, tab_width);
        lv_obj_set_style_text_font(screen_, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_add_event_cb(screen_, event_callback<ui_information_screen, &ui_information_screen::screen_callback>, LV_EVENT_ALL, this);
        lv_obj_add_event_cb(tab_view_, event_callback<ui_information_screen, &ui_information_screen::tab_view_callback>, LV_EVENT_ALL, this);

        // Wifi tab
        {
            auto tab = lv_tabview_add_tab(tab_view_, LV_SYMBOL_WIFI);
            set_padding_zero(tab);

            network_table_ = create_table(tab);
        }

        // Settings tab
        {
            auto tab = lv_tabview_add_tab(tab_view_, LV_SYMBOL_SETTINGS);
            set_padding_zero(tab);

            config_table_ = create_table(tab);
        }

        // Information tab
        {
            auto tab = lv_tabview_add_tab(tab_view_, LV_SYMBOL_LIST);
            set_padding_zero(tab);
            system_table_ = create_table(tab);
        }

        auto tab_btns = lv_tabview_get_tab_btns(tab_view_);
        lv_obj_set_style_text_font(tab_btns, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        create_close_button_to_main_screen(screen_, LV_ALIGN_TOP_RIGHT, -15, 15);
    }

    void show_screen()
    {
        ESP_LOGI(UI_TAG, "Showing information screen");
        lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }

  private:
    lv_obj_t *tab_view_{};
    lv_obj_t *network_table_{};
    lv_obj_t *config_table_{};
    lv_obj_t *system_table_{};

    lv_timer_t *refresh_timer_{};

    constexpr static int tab_width = 64;

    void screen_callback(lv_event_t *e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        if (event_code == LV_EVENT_SCREEN_LOAD_START)
        {
            ESP_LOGD(UI_TAG, "setting screen shown");
            load_information(nullptr);
            refresh_timer_ = lv_timer_create(timer_callback<ui_information_screen, &ui_information_screen::load_information>, 1000, this);
        }
        else if (event_code == LV_EVENT_SCREEN_UNLOADED)
        {
            ESP_LOGD(UI_TAG, "setting screen hidden");
            if (refresh_timer_)
            {
                lv_timer_del(refresh_timer_);
                refresh_timer_ = nullptr;
            }
        }
    }

    static lv_obj_t *__attribute__((noinline)) create_table(lv_obj_t *tab)
    {
        auto table = lv_table_create(tab);
        lv_obj_set_size(table, lv_pct(100), screen_height);

        lv_obj_set_style_border_width(table, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(table, 0, LV_PART_ITEMS);
        lv_obj_set_style_bg_opa(table, LV_OPA_0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(table, LV_OPA_0, LV_PART_ITEMS);
        return table;
    }

    void __attribute__((noinline)) tab_view_callback(lv_event_t *e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        if (event_code == LV_EVENT_VALUE_CHANGED)
        {
            load_information(nullptr);
            lv_timer_reset(refresh_timer_);
        }
    }

    void __attribute__((noinline)) load_information(lv_timer_t *)
    {
        const auto current_tab = lv_tabview_get_tab_act(tab_view_);

        if (current_tab == 2)
        {
            const auto data = ui_interface_instance_.get_information_table(ui_interface::information_type::system);
            update_table(system_table_, data, tab_width);
        }
        else if (current_tab == 1)
        {
            const auto data = ui_interface_instance_.get_information_table(ui_interface::information_type::config);
            update_table(config_table_, data, tab_width);
        }
        else if (current_tab == 0)
        {
            const auto data = ui_interface_instance_.get_information_table(ui_interface::information_type::network);
            update_table(network_table_, data, tab_width);
        }
    }
};