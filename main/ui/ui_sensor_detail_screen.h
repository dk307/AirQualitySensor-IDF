#pragma once

#include "ui_screen_with_sensor_panel.h"
#include "sensor/sensor.h"

class ui_sensor_detail_screen final : public ui_screen_with_sensor_panel
{
public:
    using ui_screen_with_sensor_panel::ui_screen_with_sensor_panel;

    void init() override
    {
        ui_screen_with_sensor_panel::init();

        const auto x_pad = 6;
        const auto y_pad = 4;

        lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        sensor_detail_screen_top_label =
            create_sensor_label(screen, fonts->font_montserrat_medium_48, LV_ALIGN_TOP_MID, 0, y_pad, text_color);

        sensor_detail_screen_top_label_units =
            create_sensor_label(screen, fonts->font_montserrat_medium_units_18, LV_ALIGN_TOP_RIGHT, -2 * x_pad, y_pad + 10, text_color);

        lv_obj_set_style_text_align(sensor_detail_screen_top_label_units, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);

        const auto top_y_margin =
            lv_obj_get_style_text_font(sensor_detail_screen_top_label, LV_PART_MAIN | LV_STATE_DEFAULT)->line_height + y_pad + 2;

        const auto panel_w = (screen_width - x_pad * 2) / 5;
        const auto panel_h = (screen_height - y_pad * 4 - top_y_margin) / 4;

        // first label is up by y_pad
        panel_and_labels[label_and_unit_label_current_index] =
            create_panel("Current",
                         LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin,
                         panel_w, panel_h);

        panel_and_labels[label_and_unit_label_average_index] =
            create_panel("Average",
                         LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin + y_pad + panel_h,
                         panel_w, panel_h);

        panel_and_labels[label_and_unit_label_min_index] =
            create_panel("Minimum",
                         LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin + (y_pad + panel_h) * 2,
                         panel_w, panel_h);

        panel_and_labels[label_and_unit_label_max_index] =
            create_panel("Maximum",
                         LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin + (y_pad + panel_h) * 3,
                         panel_w, panel_h);

        {
            const auto extra_chart_x = 45;
            sensor_detail_screen_chart = lv_chart_create(screen);
            lv_obj_set_style_bg_opa(sensor_detail_screen_chart, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_size(sensor_detail_screen_chart,
                            screen_width - panel_w - x_pad * 3 - extra_chart_x - 10,
                            screen_height - top_y_margin - 3 * y_pad - 20);
            lv_obj_align(sensor_detail_screen_chart, LV_ALIGN_TOP_LEFT, x_pad + extra_chart_x, y_pad + top_y_margin);
            lv_obj_set_style_size(sensor_detail_screen_chart, 0, LV_PART_INDICATOR);
            sensor_detail_screen_chart_series =
                lv_chart_add_series(sensor_detail_screen_chart,
                                    lv_theme_get_color_primary(sensor_detail_screen_chart),
                                    LV_CHART_AXIS_PRIMARY_Y);
            lv_obj_set_style_text_font(sensor_detail_screen_chart, fonts->font_montserrat_medium_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_chart_set_axis_tick(sensor_detail_screen_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 1, 3, 1, true, 200);
            lv_chart_set_axis_tick(sensor_detail_screen_chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, chart_total_x_ticks, 1, true, 50);

            lv_chart_set_div_line_count(sensor_detail_screen_chart, 3, 3);
            lv_obj_set_style_border_width(sensor_detail_screen_chart, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_add_event_cb(sensor_detail_screen_chart,
                                event_callback<ui_sensor_detail_screen, &ui_sensor_detail_screen::chart_draw_event_cb>,
                                LV_EVENT_DRAW_PART_BEGIN, this);
        }

        create_close_button_to_main_screen(screen, LV_ALIGN_BOTTOM_LEFT, 15, -15);
        lv_obj_add_event_cb(screen, event_callback<ui_sensor_detail_screen, &ui_sensor_detail_screen::screen_callback>, LV_EVENT_ALL, this);

        ESP_LOGD(UI_TAG, "Sensor detail init done");
    }

    void set_sensor_value(sensor_id_index index, const std::optional<sensor_value::value_type> &value)
    {
        if (is_active())
        {
            if (get_sensor_id_index() == index)
            {
                ESP_LOGI(UI_TAG, "Updating sensor %s to %d in details screen", get_sensor_name(index), value.value_or(-1));
                set_current_values(index, value);
            }
        }
    }

    sensor_id_index get_sensor_id_index()
    {
        return static_cast<sensor_id_index>(reinterpret_cast<uint64_t>(lv_obj_get_user_data(screen)));
    }

    void show_screen(sensor_id_index index)
    {
        ESP_LOGI(UI_TAG, "Panel pressed for sensor index:%s", get_sensor_name(index));
        lv_obj_set_user_data(screen, reinterpret_cast<void *>(index));

        const auto sensor_definition = get_sensor_definition(index);
        lv_label_set_text(sensor_detail_screen_top_label, sensor_definition.get_name());
        lv_label_set_text_static(sensor_detail_screen_top_label_units, sensor_definition.get_unit());

        const auto value = ui_interface_instance.get_sensor_value(index);
        set_current_values(index, value);

        lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }

private:
    lv_obj_t *sensor_detail_screen_top_label;
    lv_obj_t *sensor_detail_screen_top_label_units;
    lv_obj_t *sensor_detail_screen_chart;
    lv_chart_series_t *sensor_detail_screen_chart_series;
    sensor_history::vector_history_t sensor_detail_screen_chart_series_data;
    std::optional<time_t> sensor_detail_screen_chart_series_data_time;
    const static uint8_t chart_total_x_ticks = 4;

    std::array<panel_and_label, 4> panel_and_labels;
    const size_t label_and_unit_label_current_index = 0;
    const size_t label_and_unit_label_average_index = 1;
    const size_t label_and_unit_label_min_index = 2;
    const size_t label_and_unit_label_max_index = 3;

    void screen_callback(lv_event_t *e)
    {
        lv_event_code_t event_code = lv_event_get_code(e);

        if (event_code == LV_EVENT_GESTURE)
        {
            const auto dir = lv_indev_get_gesture_dir(lv_indev_get_act());

            if (dir == LV_DIR_LEFT)
            {
                const auto index = static_cast<int8_t>(get_sensor_id_index()) + 1;
                inter_screen_interface.show_sensor_detail_screen(
                    index > static_cast<int8_t>(sensor_id_index::last) ? sensor_id_index::first : static_cast<sensor_id_index>(index));
            }
            else if (dir == LV_DIR_RIGHT)
            {
                const auto index = static_cast<int8_t>(get_sensor_id_index()) - 1;
                inter_screen_interface.show_sensor_detail_screen(
                    index < static_cast<int8_t>(sensor_id_index::first) ? sensor_id_index::last : static_cast<sensor_id_index>(index));
            }
        }
    }

    panel_and_label create_panel(const char *label_text,
                                 lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                                 lv_coord_t w, lv_coord_t h)
    {
        auto panel = lv_obj_create(screen);
        lv_obj_set_size(panel, w, h);
        lv_obj_align(panel, align, x_ofs, y_ofs);
        lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_bg_opa(panel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(panel, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_clip_corner(panel, false, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(panel, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_set_style_radius(panel, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
        set_padding_zero(panel);

        auto current_static_label =
            create_sensor_label(panel, fonts->font_montserrat_medium_14, LV_ALIGN_TOP_MID, 0, 3, text_color);

        lv_label_set_text_static(current_static_label, label_text);

        auto value_label =
            create_sensor_label(panel, fonts->font_montserrat_regular_numbers_40, LV_ALIGN_BOTTOM_MID,
                                0, -3, text_color);

        return {panel, value_label};
    }

    static lv_obj_t *create_sensor_label(lv_obj_t *parent, const lv_font_t *font,
                                         lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, lv_color_t color)
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

    void chart_draw_event_cb(lv_event_t *e)
    {
        lv_obj_t *obj = lv_event_get_target(e);

        /*Add the faded area before the lines are drawn*/
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        if (dsc->part == LV_PART_ITEMS)
        {
            if (!dsc->p1 || !dsc->p2)
                return;

            /*Add a line mask that keeps the area below the line*/
            lv_draw_mask_line_param_t line_mask_param;
            lv_draw_mask_line_points_init(&line_mask_param, dsc->p1->x, dsc->p1->y, dsc->p2->x, dsc->p2->y,
                                          LV_DRAW_MASK_LINE_SIDE_BOTTOM);
            int16_t line_mask_id = lv_draw_mask_add(&line_mask_param, NULL);

            /*Add a fade effect: transparent bottom covering top*/
            lv_coord_t h = lv_obj_get_height(obj);
            lv_draw_mask_fade_param_t fade_mask_param;
            lv_draw_mask_fade_init(&fade_mask_param, &obj->coords, LV_OPA_COVER, obj->coords.y1 + h / 8, LV_OPA_TRANSP,
                                   obj->coords.y2);
            int16_t fade_mask_id = lv_draw_mask_add(&fade_mask_param, NULL);

            /*Draw a rectangle that will be affected by the mask*/
            lv_draw_rect_dsc_t draw_rect_dsc;
            lv_draw_rect_dsc_init(&draw_rect_dsc);
            draw_rect_dsc.bg_opa = LV_OPA_80;
            draw_rect_dsc.bg_color = dsc->line_dsc->color;

            lv_area_t a;
            a.x1 = dsc->p1->x;
            a.x2 = dsc->p2->x - 1;
            a.y1 = LV_MIN(dsc->p1->y, dsc->p2->y);
            a.y2 = obj->coords.y2;
            lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

            /*Remove the masks*/
            lv_draw_mask_free_param(&line_mask_param);
            lv_draw_mask_free_param(&fade_mask_param);
            lv_draw_mask_remove_id(line_mask_id);
            lv_draw_mask_remove_id(fade_mask_id);
        }
        /*Hook the division lines too*/
        else if (dsc->part == LV_PART_MAIN)
        {
            if (dsc->line_dsc == NULL || dsc->p1 == NULL || dsc->p2 == NULL)
                return;

            /*Vertical line*/
            if (dsc->p1->x == dsc->p2->x)
            {
                dsc->line_dsc->color = lv_palette_lighten(LV_PALETTE_GREY, 1);
                if (dsc->id == 3)
                {
                    dsc->line_dsc->width = 2;
                    dsc->line_dsc->dash_gap = 0;
                    dsc->line_dsc->dash_width = 0;
                }
                else
                {
                    dsc->line_dsc->width = 1;
                    dsc->line_dsc->dash_gap = 6;
                    dsc->line_dsc->dash_width = 6;
                }
            }
            /*Horizontal line*/
            else
            {
                if (dsc->id == 2)
                {
                    dsc->line_dsc->width = 3;
                    dsc->line_dsc->dash_gap = 0;
                    dsc->line_dsc->dash_width = 0;
                }
                else
                {
                    dsc->line_dsc->width = 2;
                    dsc->line_dsc->dash_gap = 6;
                    dsc->line_dsc->dash_width = 6;
                }

                if (dsc->id == 1 || dsc->id == 3)
                {
                    dsc->line_dsc->color = lv_color_darken(lv_theme_get_color_primary(sensor_detail_screen_chart), LV_OPA_20);
                }
                else
                {
                    dsc->line_dsc->color = lv_palette_lighten(LV_PALETTE_GREY, 1);
                }
            }
        }
        else if (dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_X)
        {
            if (sensor_detail_screen_chart_series_data_time.has_value() && sensor_detail_screen_chart_series_data.size())
            {
                const auto data_interval_seconds = (sensor_detail_screen_chart_series_data.size() * 60);
                const float interval = float(chart_total_x_ticks - 1 - dsc->value) / (chart_total_x_ticks - 1);
                // ESP_LOGI("total seconds :%d,  Series length: %d,  %f", data_interval_seconds, sensor_detail_screen_chart_series_data.size(), interval);
                const time_t tick_time = sensor_detail_screen_chart_series_data_time.value() - (data_interval_seconds * interval);

                tm t{};
                localtime_r(&tick_time, &t);
                strftime(dsc->text, 32, "%I:%M%p", &t);
            }
            else
            {
                strcpy(dsc->text, "-");
            }
        }
    }

    void set_current_values(sensor_id_index index, const std::optional<sensor_value::value_type> &value)
    {

        set_value_in_panel(panel_and_labels[label_and_unit_label_current_index], index, value);

        // sensor_detail_screen_chart_series_data_time = ntp_time::instance.get_local_time();

        time_t t = sensor_detail_screen_chart_series_data_time.value_or((time_t)0);

        auto &&sensor_info = ui_interface_instance.get_sensor_detail_info(index);

        if (sensor_info.stat.has_value())
        {
            ESP_LOGD(UI_TAG, "Found stats for %s", get_sensor_name(index));
            auto &&stats = sensor_info.stat.value();

            lv_chart_set_type(sensor_detail_screen_chart, LV_CHART_TYPE_LINE);

            set_value_in_panel(panel_and_labels[label_and_unit_label_average_index], index, stats.mean);
            set_value_in_panel(panel_and_labels[label_and_unit_label_min_index], index, stats.min);
            set_value_in_panel(panel_and_labels[label_and_unit_label_max_index], index, stats.max);

            auto &&values = sensor_info.history;
            lv_chart_set_point_count(sensor_detail_screen_chart, values.size());
            const auto range = std::max<sensor_value::value_type>((stats.max - stats.min) * 0.1, 2);
            lv_chart_set_range(sensor_detail_screen_chart, LV_CHART_AXIS_PRIMARY_Y, stats.min - range / 2, stats.max + range / 2);

            sensor_detail_screen_chart_series_data = std::move(values);

            lv_chart_set_ext_y_array(sensor_detail_screen_chart, sensor_detail_screen_chart_series,
                                     sensor_detail_screen_chart_series_data.data());
        }
        else
        {
            ESP_LOGD(UI_TAG, "No stats for %s", get_sensor_name(index));
            set_default_value_in_panel(panel_and_labels[label_and_unit_label_average_index]);
            set_default_value_in_panel(panel_and_labels[label_and_unit_label_min_index]);
            set_default_value_in_panel(panel_and_labels[label_and_unit_label_max_index]);

            sensor_detail_screen_chart_series_data.clear();
            lv_chart_set_type(sensor_detail_screen_chart, LV_CHART_TYPE_NONE);
        }
    }
};