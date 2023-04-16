#pragma once

#include "hardware/sensors/sensor.h"
#include "ui_screen.h"
#include <array>
#include <utility>

class ui_screen_with_sensor_panel : public ui_screen
{
  public:
    using ui_screen::ui_screen;

    typedef struct
    {
        lv_obj_t *panel{nullptr};
        lv_obj_t *label{nullptr};

        bool is_valid() const
        {
            return panel || label;
        }
    } panel_and_label;

    void init() override
    {
        ui_screen::init();
        set_default_screen_color();
    }

  protected:
    constexpr static uint8_t no_value_label_ = 0;

    static void __attribute__((noinline)) set_default_value_in_panel(const panel_and_label &pair)
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

    static void __attribute__((noinline)) set_label_panel_color(lv_obj_t *panel, uint8_t level)
    {
        if (level >= panel_colors.size())
        {
            level = 0;
        }

        auto &&entry = panel_colors[level];

        lv_obj_set_style_bg_color(panel, std::get<0>(entry), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(panel, std::get<1>(entry), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    static void __attribute__((noinline)) set_value_in_panel(const panel_and_label &pair, sensor_id_index index, const std::optional<int16_t> &value)
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
                    lv_label_set_text_fmt(pair.label, "%ld%.*s", std::lround(value), sensor_definition.get_unit().size(),
                                          sensor_definition.get_unit().data());
                }
                else
                {
                    lv_label_set_text_fmt(pair.label, "%ld", std::lround(value));
                }
            }
        }
        else
        {
            set_default_value_in_panel(pair);
        }
    }

    static lv_obj_t *__attribute__((noinline))
    create_sensor_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_color_t color)
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

  private:
    const lv_color_t level0_color{lv_color_hex(0xC0C0C0)}; // silver
    const lv_color_t level1_color{lv_color_hex(0x9BDE31)}; // green
    const lv_color_t level2_color{lv_color_hex(0xD8E358)}; // yellow
    const lv_color_t level3_color{lv_color_hex(0xF59D06)}; // orange
    const lv_color_t level4_color{lv_color_hex(0xF0262D)}; // red
    const lv_color_t level5_color{lv_color_hex(0xB202E8)}; // purple
    const lv_color_t level6_color{lv_color_hex(0x940606)}; // Maroon

    const std::array<std::pair<lv_color_t, lv_color_t>, 7> panel_colors{
        std::pair<lv_color_t, lv_color_t>{level0_color, lv_color_darken(level0_color, LV_OPA_50)},
        std::pair<lv_color_t, lv_color_t>{level1_color, lv_color_darken(level1_color, LV_OPA_50)},
        std::pair<lv_color_t, lv_color_t>{level2_color, lv_color_darken(level2_color, LV_OPA_50)},
        std::pair<lv_color_t, lv_color_t>{level3_color, lv_color_darken(level3_color, LV_OPA_50)},
        std::pair<lv_color_t, lv_color_t>{level4_color, lv_color_darken(level4_color, LV_OPA_50)},
        std::pair<lv_color_t, lv_color_t>{level5_color, lv_color_darken(level5_color, LV_OPA_50)},
        std::pair<lv_color_t, lv_color_t>{level6_color, lv_color_darken(level6_color, LV_OPA_50)},
    };
};
