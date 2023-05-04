#include "ui/ui_launcher_screen.h"
#include "logging/logging_tags.h"
#include "operations/operations.h"
#include <esp_log.h>

void ui_launcher_screen::init()
{
    LV_IMG_DECLARE(restart_png_img);
    LV_IMG_DECLARE(factory_reset_png_img);
    LV_IMG_DECLARE(info_png_img);
    LV_IMG_DECLARE(homekit_png_img);
    LV_IMG_DECLARE(hardware_png_img);
    LV_IMG_DECLARE(wifi_png_img);

    ui_screen::init();
    set_default_screen_color();

    lv_obj_add_event_cb(screen_, event_callback<ui_launcher_screen, &ui_launcher_screen::screen_callback>, LV_EVENT_ALL, this);

    lv_style_init(&button_style_default_);
    lv_style_set_text_color(&button_style_default_, text_color);
    lv_style_set_img_opa(&button_style_default_, LV_OPA_100);
    lv_style_set_text_font(&button_style_default_, &lv_font_montserrat_18);

    lv_style_init(&button_style_pressed_);
    lv_style_set_img_recolor_opa(&button_style_pressed_, LV_OPA_30);
    lv_style_set_img_recolor(&button_style_pressed_, text_color);
    lv_style_set_text_color(&button_style_pressed_, lv_color_lighten(text_color, LV_OPA_50));
    lv_style_set_text_font(&button_style_pressed_, &lv_font_montserrat_18);

    int pad = 30;
    create_button(LV_ALIGN_TOP_LEFT, pad, pad, &restart_png_img, "Restart", event_callback<ui_launcher_screen, &ui_launcher_screen::restart>);
    create_button(LV_ALIGN_TOP_MID, 0, pad, &factory_reset_png_img, "Factory\nReset",
                  event_callback<ui_launcher_screen, &ui_launcher_screen::factory_reset>);
    create_button(LV_ALIGN_TOP_RIGHT, -pad, pad, &info_png_img, "Information",
                  event_callback<ui_launcher_screen, &ui_launcher_screen::show_information>);
    create_button(LV_ALIGN_BOTTOM_LEFT, pad, -pad - 25, &homekit_png_img, "Homekit",
                  event_callback<ui_launcher_screen, &ui_launcher_screen::homekit_info>);
    create_button(LV_ALIGN_BOTTOM_MID, 0, -pad - 25, &hardware_png_img, "Hardware",
                  event_callback<ui_launcher_screen, &ui_launcher_screen::hardware_info>);
    create_button(LV_ALIGN_BOTTOM_RIGHT, -pad, -pad - 25, &wifi_png_img, "Wifi\nEnroll",
                  event_callback<ui_launcher_screen, &ui_launcher_screen::wifi_setup>);

    init_confirm_win();

    create_close_button_to_main_screen(screen_, LV_ALIGN_TOP_LEFT, 15, 15);
}

void ui_launcher_screen::show_screen()
{
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void ui_launcher_screen::screen_callback(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if ((event_code == LV_EVENT_SCREEN_LOAD_START) || (event_code == LV_EVENT_SCREEN_UNLOADED))
    {
        lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_launcher_screen::create_button(lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const lv_img_dsc_t *src, const char *label_str,
                                            lv_event_cb_t event_cb)
{
    auto btn = lv_imgbtn_create(screen_);
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(btn, align, x_ofs, y_ofs);
    lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, src, NULL);

    lv_obj_add_style(btn, &button_style_default_, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(btn, &button_style_pressed_, LV_STATE_PRESSED);

    auto label = lv_label_create(screen_);
    lv_label_set_text_static(label, label_str);
    lv_obj_align_to(label, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_style(label, &button_style_default_, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(label, &button_style_pressed_, LV_STATE_PRESSED);

    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(label, event_cb, LV_EVENT_CLICKED, this);

    return btn;
}

void ui_launcher_screen::init_confirm_win()
{
    win_confirm_ = lv_win_create(screen_, 35);
    lv_win_add_title(win_confirm_, "Confirm");
    lv_obj_set_size(win_confirm_, 2 * screen_width / 3, screen_height / 2);

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

    lv_obj_add_event_cb(btn_yes, event_callback<ui_launcher_screen, &ui_launcher_screen::yes_win_confirm>, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn_no, event_callback<ui_launcher_screen, &ui_launcher_screen::close_win_confirm>, LV_EVENT_CLICKED, this);

    lv_obj_center(win_confirm_);
    lv_obj_align(win_confirm_label_, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_align(btn_yes, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    lv_obj_align(btn_no, LV_ALIGN_BOTTOM_RIGHT, -5, -5);

    lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
}

void ui_launcher_screen::show_information(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "Showing information clicked");
    inter_screen_interface_.show_setting_screen();
}

void ui_launcher_screen::hardware_info(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "Showing Hardware clicked");
    inter_screen_interface_.show_hardware_info_screen();
}

void ui_launcher_screen::homekit_info(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "Showing Homekit screen");
    inter_screen_interface_.show_homekit_screen();
}

void ui_launcher_screen::wifi_setup(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "Starting wifi enrollment");
    inter_screen_interface_.show_wifi_enroll_screen();
}

void ui_launcher_screen::restart(lv_event_t *)
{
    ESP_LOGI(UI_TAG, "Restart clicked");
    show_confirm(confirm_type::restart);
}

void ui_launcher_screen::factory_reset(lv_event_t *)
{
    ESP_LOGI(UI_TAG, "Factory Reset");
    show_confirm(confirm_type::factory_reset);
}

void ui_launcher_screen::yes_win_confirm(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "Yes clicked");
    lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
    confirm_type type = (confirm_type)(uintptr_t)lv_obj_get_user_data(win_confirm_);

    switch (type)
    {
    case confirm_type::restart:
        operations::instance.reboot();
        break;
    case confirm_type::factory_reset:
        operations::instance.factory_reset();
        break;
    }
}

void ui_launcher_screen::close_win_confirm(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "No clicked");
    lv_obj_add_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
}

void ui_launcher_screen::show_confirm(confirm_type type)
{
    lv_obj_set_user_data(win_confirm_, (void *)type);
    switch (type)
    {
    case confirm_type::restart:
        lv_label_set_text_static(win_confirm_label_, "Do you want to restart?");
        break;
    case confirm_type::factory_reset:
        lv_label_set_text_static(win_confirm_label_, "#ff0000 Do you want to factory reset?#");
        break;
    }
    lv_obj_clear_flag(win_confirm_, LV_OBJ_FLAG_HIDDEN);
}
