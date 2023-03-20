#pragma once

#include "sensor/sensor.h"
#include "ui_screen.h"

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
    const static uint8_t no_value_label_ = std::numeric_limits<uint8_t>::max();

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
        uint32_t color;
        uint32_t color_grad;

        switch (level)
        {
        default:
        case 0:
            color = 0x9BDE31; // green
            color_grad = 0x7EB528;
            break;
        case 1:
            color = 0xD8E358; // yellow
            color_grad = 0xA3AB42;
            break;
        case 2:
            color = 0xF59D06; // orange
            color_grad = 0xC98105;
            break;
        case 3:
            color = 0xF0262D; // red
            color_grad = 0x9E191E;
            break;
        case 4:
            color = 0xB202E8; // purple
            color_grad = 0x9301BF;
            break;
        case 5:
            color = 0x940606; // Maroon
            color_grad = 0xc30808;
            break;
        case no_value_label_:
            color = 0xC0C0C0; // Silver
            color_grad = 0x696969;
            break;
        }

        lv_obj_set_style_bg_color(panel, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(panel, lv_color_hex(color_grad), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    static void __attribute__((noinline)) set_value_in_panel(const panel_and_label &pair, sensor_id_index index, const std::optional<int16_t> &value)
    {
        if (value.has_value())
        {
            const auto final_value = value.value();
            const auto sensor_definition = get_sensor_definition(index);
            if (pair.panel)
            {
                const auto level = sensor_definition.calculate_level(final_value);
                set_label_panel_color(pair.panel, level);
            }

            if (pair.label)
            {
                if (!pair.panel)
                {
                    lv_label_set_text_fmt(pair.label, "%d%.*s", final_value, sensor_definition.get_unit().size(),
                                          sensor_definition.get_unit().data());
                }
                else
                {
                    lv_label_set_text_fmt(pair.label, "%d", final_value);
                }
            }
        }
        else
        {
            set_default_value_in_panel(pair);
        }
    }
};
