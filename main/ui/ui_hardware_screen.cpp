#include "ui/ui_hardware_screen.h"
#include "logging/logging_tags.h"
#include "operations/operations.h"
#include <esp_log.h>

void ui_hardware_screen::init()
{
    ui_screen::init();
    set_default_screen_color();

    lv_obj_add_event_cb(screen_, event_callback<ui_hardware_screen, &ui_hardware_screen::screen_callback>, LV_EVENT_ALL, this);

    create_screen_title(4, "Hardware");

    // baseline
    auto btn_clean = create_btn("Clean SPS 30", event_callback<ui_hardware_screen, &ui_hardware_screen::clean_sps_30>);
    lv_obj_align(btn_clean, LV_ALIGN_TOP_MID, 0, 105);

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
    auto reset_scd4x = create_btn("Factory Reset SCD 4x", event_callback<ui_hardware_screen, &ui_hardware_screen::factory_reset_scd4x>);
    lv_obj_align_to(reset_scd4x, btn_clean, LV_ALIGN_OUT_BOTTOM_MID, 0, 50);
#endif

    init_status_win();
    create_close_button_to_main_screen(screen_, LV_ALIGN_TOP_LEFT, 15, 15);
}

void ui_hardware_screen::show_screen()
{
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void ui_hardware_screen::screen_callback(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if ((event_code == LV_EVENT_SCREEN_LOAD_START) || (event_code == LV_EVENT_SCREEN_UNLOADED))
    {
        lv_obj_add_flag(win_status_, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_hardware_screen::create_btn(const char *label_text, lv_event_cb_t event_cb)
{
    auto btn_set_baseline = lv_btn_create(screen_);
    lv_obj_add_event_cb(btn_set_baseline, event_cb, LV_EVENT_PRESSED, this);
    lv_obj_set_height(btn_set_baseline, LV_SIZE_CONTENT);

    auto label = lv_label_create(btn_set_baseline);
    lv_label_set_text_static(label, label_text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

    return btn_set_baseline;
}

void ui_hardware_screen::init_status_win()
{
    win_status_ = lv_win_create(screen_, 35);
    lv_win_add_title(win_status_, "Status");
    lv_obj_set_size(win_status_, 2 * screen_width / 3, screen_height / 2);

    auto cont = lv_win_get_content(win_status_);
    win_status_label_ = lv_label_create(cont);

    auto btn_close = lv_btn_create(cont);
    auto btn_close_label = lv_label_create(btn_close);
    lv_label_set_text_static(btn_close_label, "Close");
    lv_obj_center(btn_close_label);

    lv_obj_add_event_cb(btn_close, event_callback<ui_hardware_screen, &ui_hardware_screen::close_win_confirm>, LV_EVENT_CLICKED, this);

    lv_obj_center(win_status_);
    lv_obj_align(win_status_label_, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_align(btn_close, LV_ALIGN_BOTTOM_MID, 0, -5);

    lv_obj_add_flag(win_status_, LV_OBJ_FLAG_HIDDEN);
}

void ui_hardware_screen::close_win_confirm(lv_event_t *e)
{
    ESP_LOGI(UI_TAG, "Close clicked");
    if (confirm_close_action)
    {
        confirm_close_action();
        confirm_close_action = nullptr;
    }
    lv_obj_add_flag(win_status_, LV_OBJ_FLAG_HIDDEN);
}

void ui_hardware_screen::clean_sps_30(lv_event_t *e)
{
    const auto code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED)
    {
        const bool success = ui_interface_instance_.clean_sps_30();
        lv_label_set_text_static(win_status_label_, success ? "Cleanup started." : "Clean start failed.");
        lv_obj_clear_flag(win_status_, LV_OBJ_FLAG_HIDDEN);
    }
}

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
void ui_hardware_screen::factory_reset_scd4x(lv_event_t *e)
{
    const auto code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED)
    {
        const bool success = ui_interface_instance_.factory_reset_scd4x();
        lv_label_set_text_static(win_status_label_, success ? "Factory Reset Done.\nRestart Required" : "Failed to Reset.\nRestart Required");
        confirm_close_action = [this] { operations::instance.reboot(); };
        lv_obj_clear_flag(win_status_, LV_OBJ_FLAG_HIDDEN);
    }
}
#endif