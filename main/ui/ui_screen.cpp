

#include "ui/ui_screen.h"
#include "logging/logging_tags.h"
#include "ui/ui_inter_screen_interface.h"
#include "ui/ui_interface.h"
#include "util/noncopyable.h"
#include <esp_log.h>
#include <lvgl.h>

ui_screen::ui_screen(config &config, ui_interface &ui_interface_instance_, ui_inter_screen_interface &ui_inter_screen_interface)
    : config_(config), ui_interface_instance_(ui_interface_instance_), inter_screen_interface_(ui_inter_screen_interface)
{
}

void ui_screen::init()
{
    screen_ = lv_obj_create(NULL);
}

bool ui_screen::is_active() const
{
    return lv_scr_act() == screen_;
}

void ui_screen::set_padding_zero(lv_obj_t *obj)
{
    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_screen::create_close_button_to_main_screen(lv_obj_t *parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)
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

void ui_screen::set_default_screen_color()
{
    lv_obj_set_style_bg_grad_dir(screen_, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x080402), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(screen_, lv_color_hex(0x060606), LV_PART_MAIN | LV_STATE_DEFAULT);
}

lv_obj_t *ui_screen::create_a_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    auto *label = lv_label_create(parent);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(label, align, x_ofs, y_ofs);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);

    return label;
}

void ui_screen::update_table(lv_obj_t *table, const ui_interface::information_table_type &data, uint8_t col_diff)
{
    lv_table_set_col_cnt(table, 2);
    lv_table_set_row_cnt(table, data.size());

    lv_table_set_col_width(table, 0, 140);
    lv_table_set_col_width(table, 1, screen_width - col_diff - 140 - 5); // 5 for borders ,etc

    for (auto i = 0; i < data.size(); i++)
    {
        lv_table_set_cell_value(table, i, 0, std::get<0>(data[i]).data());
        lv_table_set_cell_value(table, i, 1, std::get<1>(data[i]).c_str());
    }
}

lv_obj_t *ui_screen::create_screen_title(lv_coord_t y_ofs, const char *title)
{
    auto label = create_a_label(screen_, &all_48_font, LV_ALIGN_TOP_MID, 0, y_ofs);
    lv_label_set_text_static(label, title);
    return label;
}

void ui_screen::event_callback_ftn(lv_event_t *e)
{
    auto p_ftn = reinterpret_cast<std::function<void(lv_event_t * e)> *>(lv_event_get_user_data(e));
    (*p_ftn)(e);
}

void ui_screen::close_button_callback(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_SHORT_CLICKED)
    {
        inter_screen_interface_.show_home_screen();
    }
}
