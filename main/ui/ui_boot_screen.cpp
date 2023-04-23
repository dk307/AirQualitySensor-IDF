#include "ui_boot_screen.h"

extern lv_img_dsc_t logo_img;

void ui_boot_screen::init()
{
    ui_screen::init();

    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(screen_, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);

    LV_IMG_DECLARE(logo_png_img);

    auto boot_logo = lv_img_create(screen_);
    lv_img_set_src(boot_logo, &logo_png_img);
    lv_obj_set_size(boot_logo, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(boot_logo, LV_ALIGN_CENTER, 0, -20);

    auto boot_message = lv_label_create(screen_);
    lv_obj_set_size(boot_message, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(boot_message, LV_ALIGN_CENTER, 0, 60);
    lv_label_set_text_static(boot_message, "Starting");
    lv_obj_set_style_text_color(boot_message, lv_color_hex(0xFCFEFC), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(boot_message, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_boot_screen::show_screen()
{
    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}
