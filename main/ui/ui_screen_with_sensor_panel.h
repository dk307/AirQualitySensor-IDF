#pragma once

#include "hardware/sensors/sensor.h"
#include "ui/ui_screen.h"
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

    void init() override;

  protected:
    void set_default_value_in_panel(const panel_and_label &pair);
    void set_panel_label_color(const panel_and_label &pair, sensor_level level);
    void set_value_in_panel(const panel_and_label &pair, sensor_id_index index, float value);
    static lv_obj_t *create_sensor_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                                         lv_color_t color);

    const lv_color_t night_mode_labels_text_color = lv_color_hex(0x929292);
    const lv_color_t day_mode_labels_text_color = lv_color_hex(0xEDEADE);
    const lv_color_t night_mode_panel_bg_start = lv_color_hex(0x080806); 
    const lv_color_t night_mode_panel_bg_end = lv_color_hex(0x0C0404); 

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
