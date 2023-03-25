#pragma once

#include "generated/display/include/logo.png.h"
#include "ui_screen.h"

extern lv_img_dsc_t logo_img;

class ui_boot_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;

    void init() override
    {
        ui_screen::init();

        lv_obj_clear_flag(screen_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(screen_, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_color(screen_, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);

        auto boot_logo = lv_img_create(screen_);
        logo_img.header.cf = LV_IMG_CF_RAW_ALPHA;
        logo_img.header.always_zero = 0;
        logo_img.header.reserved = 0;
        logo_img.header.w = 100;
        logo_img.header.h = 100;
        logo_img.data_size = 2624;
        logo_img.data = logo_map;

        lv_img_set_src(boot_logo, &logo_img);
        lv_obj_set_size(boot_logo, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(boot_logo, LV_ALIGN_CENTER, 0, -20);

        auto boot_message = lv_label_create(screen_);
        lv_obj_set_size(boot_message, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_align(boot_message, LV_ALIGN_CENTER, 0, 60);
        lv_label_set_text_static(boot_message, "Starting");
        lv_obj_set_style_text_color(boot_message, lv_color_hex(0xFCFEFC), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(boot_message, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    void show_screen()
    {
        lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }
};