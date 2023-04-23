#pragma once

#include "hardware/sensors/sensor.h"
#include "ui/ui_screen_with_sensor_panel.h"

class ui_sensor_detail_screen final : public ui_screen_with_sensor_panel
{
  public:
    using ui_screen_with_sensor_panel::ui_screen_with_sensor_panel;

    void init() override;
    void set_sensor_value(sensor_id_index index, float value);
    sensor_id_index get_sensor_id_index();
    void show_screen(sensor_id_index index);

  private:
    lv_obj_t *sensor_detail_screen_top_label{};
    lv_obj_t *sensor_detail_screen_top_label_units{};
    lv_obj_t *sensor_detail_screen_chart{};
    lv_chart_series_t *sensor_detail_screen_chart_series{};
    std::vector<lv_coord_t> sensor_detail_screen_chart_series_data;
    uint64_t sensor_detail_screen_chart_series_time;
    constexpr static uint8_t chart_total_x_ticks = 4;

    std::array<panel_and_label, 4> panel_and_labels_;
    const size_t label_and_unit_label_current_index = 0;
    const size_t label_and_unit_label_average_index = 1;
    const size_t label_and_unit_label_min_index = 2;
    const size_t label_and_unit_label_max_index = 3;
    const uint8_t graph_multiplier = 10;

    void screen_callback(lv_event_t *e);
    panel_and_label create_panel(const char *label_text, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_coord_t w, lv_coord_t h);
    void chart_draw_event_cb(lv_event_t *e);
    void set_current_values(sensor_id_index index, float value);
    void seconds_to_timestring(int seconds, char *buffer, uint32_t buffer_len);
};