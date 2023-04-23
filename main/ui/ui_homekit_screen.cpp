
#include "ui/ui_homekit_screen.h"
#include "logging/logging_tags.h"
#include <esp_log.h>

void ui_homekit_screen::init()
{
    ui_screen::init();
    set_default_screen_color();

    create_screen_title(4, "Homekit");

    lv_obj_add_event_cb(screen_, event_callback<ui_homekit_screen, &ui_homekit_screen::screen_callback>, LV_EVENT_ALL, this);

    homekit_table_ = lv_table_create(screen_);
    lv_obj_set_size(homekit_table_, lv_pct(75), screen_height / 2);
    lv_obj_align(homekit_table_, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_set_style_border_width(homekit_table_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(homekit_table_, 0, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(homekit_table_, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(homekit_table_, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_text_font(homekit_table_, &lv_font_montserrat_18, LV_PART_ITEMS);

    set_padding_zero(homekit_table_);

    auto forgot_btn = create_btn("Forget\nPairing", event_callback<ui_homekit_screen, &ui_homekit_screen::forgot_homekit_pairing>);
    lv_obj_align(forgot_btn, LV_ALIGN_BOTTOM_RIGHT, -25, -25);

    auto enable_btn = create_btn("Re-enable\nPairing", event_callback<ui_homekit_screen, &ui_homekit_screen::reenable_homekit_pairing>);
    lv_obj_align(enable_btn, LV_ALIGN_BOTTOM_LEFT, 25, -25);

    init_status_win();

    create_close_button_to_main_screen(screen_, LV_ALIGN_TOP_LEFT, 15, 15);
}

void ui_homekit_screen::show_screen()
{
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void ui_homekit_screen::screen_callback(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if ((event_code == LV_EVENT_SCREEN_LOAD_START) || (event_code == LV_EVENT_SCREEN_UNLOADED))
    {
        lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
    }

    if (event_code == LV_EVENT_SCREEN_LOAD_START)
    {
        ESP_LOGD(UI_TAG, "setting screen shown");
        load_information(nullptr);
        refresh_timer_ = lv_timer_create(timer_callback<ui_homekit_screen, &ui_homekit_screen::load_information>, 1000, this);
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

lv_obj_t *ui_homekit_screen::create_btn(const char *label_text, lv_event_cb_t event_cb)
{
    auto btn_set_baseline = lv_btn_create(screen_);
    lv_obj_add_event_cb(btn_set_baseline, event_cb, LV_EVENT_PRESSED, this);
    lv_obj_set_height(btn_set_baseline, LV_SIZE_CONTENT);
    lv_obj_set_width(btn_set_baseline, screen_width / 3);

    auto label = lv_label_create(btn_set_baseline);
    lv_label_set_text_static(label, label_text);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

    return btn_set_baseline;
}

void ui_homekit_screen::init_status_win()
{
    win_confirm_ = lv_win_create(screen_, 35);
    lv_win_add_title(win_confirm_, "Confirm");
    lv_obj_set_size(win_confirm_, 3 * screen_width / 4, screen_height / 2);

    auto cont = lv_win_get_content(win_confirm_);
    win_confirm_label_ = lv_label_create(cont);
    lv_label_set_recolor(win_confirm_label_, true);

    auto btn_yes = lv_btn_create(cont);
    auto btn_yes_label = lv_label_create(btn_yes);
    lv_label_set_text_static(btn_yes_label, "Yes");
    lv_obj_center(btn_yes_label);

    auto btn_no = lv_btn_create(cont);
    auto btn_no_label = lv_label_create(btn_no);
    lv_label_set_text_static(btn_no_label, "No");
    lv_obj_center(btn_no_label);

    lv_obj_add_event_cb(btn_yes, event_callback<ui_homekit_screen, &ui_homekit_screen::yes_win_confirm>, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn_no, event_callback<ui_homekit_screen, &ui_homekit_screen::close_win_confirm>, LV_EVENT_CLICKED, this);

    lv_obj_center(win_confirm_);
    lv_obj_align(win_confirm_label_, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_align(btn_yes, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    lv_obj_align(btn_no, LV_ALIGN_BOTTOM_RIGHT, -5, -5);

    lv_label_set_text_static(win_confirm_label_, "#ff0000 Do you want to reset homekit pairing?#");

    lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
}

void ui_homekit_screen::load_information(lv_timer_t *)
{
    const auto data = ui_interface_instance_.get_information_table(ui_interface::information_type::homekit);
    update_table(homekit_table_, data, screen_width * 0.35);
}

void ui_homekit_screen::yes_win_confirm(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "Yes clicked");
    lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
    ui_interface_instance_.forget_homekit_pairings();
}

void ui_homekit_screen::close_win_confirm(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "No clicked");
    lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
}

void ui_homekit_screen::forgot_homekit_pairing(lv_event_t *e)
{
    const auto code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED)
    {
        lv_obj_clear_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_homekit_screen::reenable_homekit_pairing(lv_event_t *e)
{
    const auto code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED)
    {
        ui_interface_instance_.reenable_homekit_pairing();
    }
}