#pragma once

#include "ui_screen.h"
// #include "config_manager.h"
 
class ui_information_screen : public ui_screen
{
public:
    using ui_screen::ui_screen;
    void init() override
    {
        ui_screen::init();
        set_default_screen();

        tab_view = lv_tabview_create(screen, LV_DIR_LEFT, tab_width);
        lv_obj_set_style_text_font(screen, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_add_event_cb(screen, event_callback<ui_information_screen, &ui_information_screen::screen_callback>, LV_EVENT_ALL, this);
        lv_obj_add_event_cb(tab_view, event_callback<ui_information_screen, &ui_information_screen::tab_view_callback>, LV_EVENT_ALL, this);

        // Wifi tab
        {
            auto tab = lv_tabview_add_tab(tab_view, LV_SYMBOL_WIFI);
            set_padding_zero(tab);

            network_table = lv_table_create(tab);
            lv_obj_set_size(network_table, lv_pct(100), screen_width - tab_width);
        }

        // Homekit tab
        {
            auto tab = lv_tabview_add_tab(tab_view, LV_SYMBOL_CALL);
            set_padding_zero(tab);
        }

        // Settings tab
        {
            auto tab = lv_tabview_add_tab(tab_view, LV_SYMBOL_SETTINGS);
            set_padding_zero(tab);

            config_table = lv_table_create(tab);
            lv_obj_set_size(config_table, lv_pct(100), screen_width - tab_width);
        }

        // Information tab
        {
            auto tab = lv_tabview_add_tab(tab_view, LV_SYMBOL_LIST);
            set_padding_zero(tab);
            system_table = lv_table_create(tab);
            lv_obj_set_size(system_table, lv_pct(100), screen_width - tab_width);
        }

        auto tab_btns = lv_tabview_get_tab_btns(tab_view);
        lv_obj_set_style_text_font(tab_btns, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
        create_close_button_to_main_screen(screen, LV_ALIGN_TOP_RIGHT, -15, 15);
    }

    void show_screen()
    {
        ESP_LOGI(UI_TAG, "Showing information screen");
        lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }

private:
    lv_obj_t *tab_view;
    lv_obj_t *network_table;
    lv_obj_t *config_table;
    lv_obj_t *system_table;

    lv_timer_t *refresh_timer;

    const static int tab_width = 64;

    void screen_callback(lv_event_t *e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        if (event_code == LV_EVENT_SCREEN_LOAD_START)
        {
            ESP_LOGD(UI_TAG, "setting screen shown");
            load_information(nullptr);
            refresh_timer = lv_timer_create(timer_callback<ui_information_screen, &ui_information_screen::load_information>, 1000, this);
        }
        else if (event_code == LV_EVENT_SCREEN_UNLOADED)
        {
            ESP_LOGD(UI_TAG, "setting screen hidden");
            if (refresh_timer)
            {
                lv_timer_del(refresh_timer);
                refresh_timer = nullptr;
            }
        }
    }

    void tab_view_callback(lv_event_t *e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);
        if (event_code == LV_EVENT_VALUE_CHANGED)
        {
            load_information(nullptr);
            lv_timer_reset(refresh_timer);
        }
    }

    static void update_table(lv_obj_t *table, const ui_interface::information_table_type &data)
    {
        lv_table_set_col_cnt(table, 2);
        lv_table_set_row_cnt(table, data.size());

        lv_table_set_col_width(table, 0, 140);
        lv_table_set_col_width(table, 1, screen_width - tab_width - 140 - 5); // 5 for borders ,etc

        for (auto i = 0; i < data.size(); i++)
        {
            lv_table_set_cell_value(table, i, 0, std::get<0>(data[i]).c_str());
            lv_table_set_cell_value(table, i, 1, std::get<1>(data[i]).c_str());
        }
    }

    void load_information(lv_timer_t *)
    {
        const auto current_tab = lv_tabview_get_tab_act(tab_view);
        if (current_tab == 3)
        {
            const auto data = ui_interface_instance.get_information_table(ui_interface::information_type::system);
            update_table(system_table, data);
        }
        else if (current_tab == 2)
        {
            const auto data = ui_interface_instance.get_information_table(ui_interface::information_type::config);
            update_table(config_table, data);
        }
        else if (current_tab == 0)
        {
            const auto data = ui_interface_instance.get_information_table(ui_interface::information_type::network);
            update_table(network_table, data);
        }
    }
};