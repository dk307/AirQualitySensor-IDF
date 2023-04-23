#include "ui/ui_screen_with_sensor_panel.h"

void ui_screen_with_sensor_panel::init()
{
    ui_screen::init();
    set_default_screen_color();
}

void ui_screen_with_sensor_panel::set_default_value_in_panel(const panel_and_label &pair)
{
    if (pair.panel)
    {
        set_label_panel_color(pair.panel, no_value_label_);
    }

    if (pair.label)
    {
        lv_label_set_text_static(pair.label, "-");
    }
}

void ui_screen_with_sensor_panel::set_label_panel_color(lv_obj_t *panel, uint8_t level)
{
    if (level >= panel_colors.size())
    {
        level = 0;
    }

    auto &&entry = panel_colors[level];

    lv_obj_set_style_bg_color(panel, std::get<0>(entry), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(panel, std::get<1>(entry), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_screen_with_sensor_panel::set_value_in_panel(const panel_and_label &pair, sensor_id_index index, float value)
{
    if (!std::isnan(value))
    {
        const auto sensor_definition = get_sensor_definition(index);
        if (pair.panel)
        {
            const auto level = sensor_definition.calculate_level(value);
            set_label_panel_color(pair.panel, level);
        }

        if (pair.label)
        {
            if (!pair.panel)
            {
                lv_label_set_text_fmt(pair.label, "%ld%.*s", std::lroundf(value), sensor_definition.get_unit().size(),
                                      sensor_definition.get_unit().data());
            }
            else
            {
                lv_label_set_text_fmt(pair.label, "%ld", std::lroundf(value));
            }
        }
    }
    else
    {
        set_default_value_in_panel(pair);
    }
}

lv_obj_t *ui_screen_with_sensor_panel::create_sensor_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs,
                                                           lv_coord_t y_ofs, lv_color_t color)
{
    auto *label = lv_label_create(parent);
    lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(label, align, x_ofs, y_ofs);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN | LV_STATE_DEFAULT);

    return label;
}
