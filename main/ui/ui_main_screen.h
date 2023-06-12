#pragma once

#include "hardware/sensors/sensor.h"
#include "ui/ui_screen_with_sensor_panel.h"

class ui_main_screen final : public ui_screen_with_sensor_panel
{
  public:
    using ui_screen_with_sensor_panel::ui_screen_with_sensor_panel;

    void init() override;
    void set_sensor_value(sensor_id_index index, float value);
    void show_screen();

    void theme_changed() override;

  private:
    panel_and_label pm_2_5_panel_and_labels_;
#if defined CONFIG_SCD30_SENSOR_ENABLE || defined CONFIG_SCD4x_SENSOR_ENABLE
    panel_and_label co2_panel_and_labels_;
#endif
    panel_and_label temperature_panel_and_labels_;
    panel_and_label humidity_panel_and_labels_;

    panel_and_label create_big_panel(sensor_id_index index, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_coord_t w, lv_coord_t h, const lv_font_t *font,
                                     lv_coord_t value_label_bottom_pad);
    lv_obj_t *create_panel(lv_coord_t x_ofs, lv_coord_t y_ofs, lv_coord_t w, lv_coord_t h, lv_coord_t radius);
    panel_and_label create_temperature_panel(lv_coord_t x_ofs, lv_coord_t y_ofs);
    panel_and_label create_humidity_panel(lv_coord_t x_ofs, lv_coord_t y_ofs);
    void screen_callback(lv_event_t *e);
    template <sensor_id_index index> void panel_callback_event(lv_event_t *e)
    {
        const auto code = lv_event_get_code(e);
        if (code == LV_EVENT_SHORT_CLICKED)
        {
            inter_screen_interface_.show_sensor_detail_screen(index);
        }
    }

    void panel_temperature_callback_event(lv_event_t *e);
    sensor_id_index get_temperature_sensor_id_index();
};