#include "ui2.h"
#include "ui_interface.h"
#include <memory>
#include <tuple>

lv_img_dsc_t logo_img;

void lv_logger(const char *dsc)
{
    ESP_LOGI(UI_TAG, "%s", dsc);
}

lv_font_t *ui::lv_font_from_sd_card(const char *path)
{
    auto font = lv_font_load(path);

    if (!font)
    {
        lv_fs_file_t file;
        lv_fs_res_t res = lv_fs_open(&file, path, LV_FS_MODE_RD);
        if (res != LV_FS_RES_OK)
        {
            ESP_LOGE(UI_TAG, "Failed to load file:%s with %d", path, res);
            return NULL;
        }

        lv_fs_close(&file);
    }
    else
    {
        ESP_LOGI(UI_TAG, "Loaded file:%s from sd card", path);
    }

    return font;
}

void ui::check_sd_card_ready()
{
    if (lv_fs_is_ready('S'))
    {
        ESP_LOGI(UI_TAG, "lv fs is ready. Loading from SD Card");
    }
    else
    {
        ESP_LOGE(UI_TAG, "lv fs not ready");
        CHECK_THROW_ESP2(ESP_FAIL, "SD Card is not ready");
    }
}

void ui::no_wifi_img_animation_cb(void *var, int32_t v)
{
    auto pThis = reinterpret_cast<ui *>(var);
    const auto op = v > 256 ? 512 - v : v;
    lv_style_set_img_opa(&pThis->no_wifi_image_style, op);
    lv_obj_refresh_style(pThis->no_wifi_image_, LV_PART_ANY, LV_STYLE_PROP_ANY);
}

void ui::load_boot_screen()
{
    check_sd_card_ready();

#if LV_USE_LOG
    lv_log_register_print_cb(&lv_logger);
#endif
    lv_disp_t *dispp = lv_disp_get_default();

    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_LIME), true, LV_FONT_DEFAULT);

    lv_disp_set_theme(dispp, theme);

    boot_screen_.init();
    boot_screen_.show_screen();

    ESP_LOGI(UI_TAG, "Loaded boot screen");
}

void ui::init()
{
    // dont't cache boot screen
    lv_img_cache_invalidate_src(NULL);

    init_top_message();
    init_no_wifi_image();

    ESP_LOGI(UI_TAG, "Initializing UI screens");
    main_screen_.init();
    launcher_screen_.init();
    sensor_detail_screen_.init();
    settings_screen_.init();
    hardware_screen_.init();
    homekit_screen_.init();
    wifi_enroll_screen_.init();
    ESP_LOGI(UI_TAG, "UI screens initialized");
}

void ui::init_no_wifi_image()
{
    no_wifi_image_ = lv_img_create(lv_layer_sys());
    lv_img_set_src(no_wifi_image_, "S:display/image/nowifi.png");
    lv_obj_align(no_wifi_image_, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_flag(no_wifi_image_, LV_OBJ_FLAG_HIDDEN);

    lv_style_init(&no_wifi_image_style);
    lv_style_set_img_opa(&no_wifi_image_style, 100);
    lv_obj_add_style(no_wifi_image_, &no_wifi_image_style, 0);

    lv_anim_t no_wifi_image_animation;
    lv_anim_init(&no_wifi_image_animation);
    lv_anim_set_var(&no_wifi_image_animation, this);
    lv_anim_set_values(&no_wifi_image_animation, 0, 512);
    lv_anim_set_time(&no_wifi_image_animation, 2000);
    lv_anim_set_exec_cb(&no_wifi_image_animation, no_wifi_img_animation_cb);
    lv_anim_set_repeat_count(&no_wifi_image_animation, LV_ANIM_REPEAT_INFINITE);

    no_wifi_image_animation_timeline_ = lv_anim_timeline_create();
    lv_anim_timeline_add(no_wifi_image_animation_timeline_, 0, &no_wifi_image_animation);
}

void ui::top_message_timer_cb(lv_timer_t *e)
{
    auto p_this = reinterpret_cast<ui *>(e->user_data);
    lv_obj_add_flag(p_this->top_message_panel_, LV_OBJ_FLAG_HIDDEN);
    lv_timer_pause(p_this->top_message_timer_);
}

void ui::init_top_message()
{
    top_message_panel_ = lv_obj_create(lv_layer_sys());
    lv_obj_set_size(top_message_panel_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(top_message_panel_, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_border_width(top_message_panel_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(top_message_panel_, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(top_message_panel_, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(top_message_panel_, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(top_message_panel_, lv_color_hex(0xF5F5F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(top_message_panel_, LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_GESTURE_BUBBLE);

    top_message_label_ = lv_label_create(top_message_panel_);
    lv_obj_set_size(top_message_panel_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(top_message_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_long_mode(top_message_label_, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(top_message_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(top_message_label_, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(top_message_label_, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);

    top_message_timer_ = lv_timer_create(top_message_timer_cb, top_message_timer_period, this);
    lv_timer_pause(top_message_timer_);
}

void ui::set_sensor_value(sensor_id_index index, const std::optional<int16_t> &value)
{
    if (main_screen_.is_active())
    {
        main_screen_.set_sensor_value(index, value);
    }
    else if (sensor_detail_screen_.is_active())
    {
        sensor_detail_screen_.set_sensor_value(index, value);
    }
}

void ui::show_top_level_message(const std::string &message, uint32_t period)
{
    ESP_LOGI(UI_TAG, "Showing top level message:%s", message.c_str());
    lv_label_set_text(top_message_label_, message.c_str());
    lv_obj_clear_flag(top_message_panel_, LV_OBJ_FLAG_HIDDEN);
    lv_timer_reset(top_message_timer_);
    lv_timer_resume(top_message_timer_);
}

void ui::set_main_screen()
{
    main_screen_.show_screen();
    wifi_changed();
}

void ui::wifi_changed()
{
    if (!boot_screen_.is_active())
    {
        const auto wifi_status = ui_interface_instance_.get_wifi_status();
        if (wifi_status.connected)
        {
            ESP_LOGI(UI_TAG, "Hiding No wifi icon");
            lv_obj_add_flag(no_wifi_image_, LV_OBJ_FLAG_HIDDEN);
            lv_anim_timeline_stop(no_wifi_image_animation_timeline_);
        }
        else
        {
            ESP_LOGI(UI_TAG, "Showing No wifi icon");
            lv_obj_clear_flag(no_wifi_image_, LV_OBJ_FLAG_HIDDEN);
            lv_anim_timeline_start(no_wifi_image_animation_timeline_);
        }

        show_top_level_message(wifi_status.status, 4000);

        if (wifi_enroll_screen_.is_active() && wifi_status.connected)
        {
            show_home_screen();
        }
    }
}

void ui::show_home_screen()
{
    main_screen_.show_screen();
}

void ui::show_setting_screen()
{
    settings_screen_.show_screen();
}

void ui::show_sensor_detail_screen(sensor_id_index index)
{
    sensor_detail_screen_.show_screen(index);
}

void ui::show_launcher_screen()
{
    lv_obj_add_flag(top_message_panel_, LV_OBJ_FLAG_HIDDEN);
    launcher_screen_.show_screen();
}

void ui::show_hardware_info_screen()
{
    lv_obj_add_flag(top_message_panel_, LV_OBJ_FLAG_HIDDEN);
    hardware_screen_.show_screen();
}

void ui::show_homekit_screen()
{
    lv_obj_add_flag(top_message_panel_, LV_OBJ_FLAG_HIDDEN);
    homekit_screen_.show_screen();
}

void ui::show_wifi_enroll_screen()
{
    lv_obj_add_flag(top_message_panel_, LV_OBJ_FLAG_HIDDEN);
    wifi_enroll_screen_.show_screen();
}
