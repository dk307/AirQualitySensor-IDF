#include "ui/ui_sensor_detail_screen.h"
#include "logging/logging_tags.h"
#include "util/helper.h"
#include "util/misc.h"
#include <esp_log.h>

void ui_sensor_detail_screen::init()
{
    ui_screen_with_sensor_panel::init();

    const auto x_pad = 6;
    const auto y_pad = 4;

    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);

    sensor_detail_screen_top_label = create_screen_title(y_pad, "");

    sensor_detail_screen_top_label_units = create_a_label(screen_, &uints_18_font, LV_ALIGN_TOP_RIGHT, -2 * x_pad, y_pad + 10);

    lv_obj_set_style_text_align(sensor_detail_screen_top_label_units, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);

    const auto top_y_margin = lv_obj_get_style_text_font(sensor_detail_screen_top_label, LV_PART_MAIN | LV_STATE_DEFAULT)->line_height + y_pad + 2;

    const auto panel_w = (screen_width - x_pad * 2) / 5;
    const auto panel_h = (screen_height - y_pad * 4 - top_y_margin) / 4;

    // first label is up by y_pad
    panel_and_labels_[label_and_unit_label_current_index] = create_panel("Current", LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin, panel_w, panel_h);

    panel_and_labels_[label_and_unit_label_average_index] =
        create_panel("Average", LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin + y_pad + panel_h, panel_w, panel_h);

    panel_and_labels_[label_and_unit_label_min_index] =
        create_panel("Minimum", LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin + (y_pad + panel_h) * 2, panel_w, panel_h);

    panel_and_labels_[label_and_unit_label_max_index] =
        create_panel("Maximum", LV_ALIGN_TOP_RIGHT, -x_pad, top_y_margin + (y_pad + panel_h) * 3, panel_w, panel_h);

    {
        const auto extra_chart_x = 45;
        sensor_detail_screen_chart = lv_chart_create(screen_);
        lv_obj_set_style_bg_opa(sensor_detail_screen_chart, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(sensor_detail_screen_chart, screen_width - panel_w - x_pad * 3 - extra_chart_x - 10,
                        screen_height - top_y_margin - 3 * y_pad - 20 - 5); // -5 for y for readablity
        lv_obj_align(sensor_detail_screen_chart, LV_ALIGN_TOP_LEFT, x_pad + extra_chart_x, y_pad + top_y_margin);
        lv_obj_set_style_size(sensor_detail_screen_chart, 0, LV_PART_INDICATOR);
        sensor_detail_screen_chart_series =
            lv_chart_add_series(sensor_detail_screen_chart, lv_theme_get_color_primary(sensor_detail_screen_chart), LV_CHART_AXIS_PRIMARY_Y);
        lv_obj_set_style_text_font(sensor_detail_screen_chart, &all_14_font, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_chart_set_axis_tick(sensor_detail_screen_chart, LV_CHART_AXIS_PRIMARY_Y, 5, 1, 3, 1, true, 200);
        lv_chart_set_axis_tick(sensor_detail_screen_chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, chart_total_x_ticks, 1, true, 50);

        lv_chart_set_div_line_count(sensor_detail_screen_chart, 3, 3);
        lv_obj_set_style_border_width(sensor_detail_screen_chart, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(sensor_detail_screen_chart, event_callback<ui_sensor_detail_screen, &ui_sensor_detail_screen::chart_draw_event_cb>,
                            LV_EVENT_DRAW_PART_BEGIN, this);
    }

    create_close_button_to_main_screen(screen_, LV_ALIGN_TOP_LEFT, 15, 15);
    lv_obj_add_event_cb(screen_, event_callback<ui_sensor_detail_screen, &ui_sensor_detail_screen::screen_callback>, LV_EVENT_ALL, this);

    ESP_LOGD(UI_TAG, "Sensor detail init done");
}

void ui_sensor_detail_screen::set_sensor_value(sensor_id_index index, float value)
{
    if (is_active())
    {
        if (get_sensor_id_index() == index)
        {
            ESP_LOGI(UI_TAG, "Updating sensor %.*s to %g in details screen", get_sensor_name(index).size(), get_sensor_name(index).data(), value);
            set_current_values(index, value);
        }
    }
}

sensor_id_index ui_sensor_detail_screen::get_sensor_id_index()
{
    return static_cast<sensor_id_index>(reinterpret_cast<uint64_t>(lv_obj_get_user_data(screen_)));
}

void ui_sensor_detail_screen::show_screen(sensor_id_index index)
{
    ESP_LOGI(UI_TAG, "Panel pressed for sensor index:%.*s", get_sensor_name(index).size(), get_sensor_name(index).data());
    lv_obj_set_user_data(screen_, reinterpret_cast<void *>(index));

    const auto sensor_definition = get_sensor_definition(index);
    lv_label_set_text_static(sensor_detail_screen_top_label, sensor_definition.get_name().data());
    lv_label_set_text_static(sensor_detail_screen_top_label_units, sensor_definition.get_unit().data());

    const auto value = ui_interface_instance_.get_sensor_value(index);
    set_current_values(index, value);

    lv_scr_load_anim(screen_, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void ui_sensor_detail_screen::theme_changed()
{
    set_current_values(get_sensor_id_index(), ui_interface_instance_.get_sensor_value(get_sensor_id_index()));
}

void ui_sensor_detail_screen::screen_callback(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);

    if (event_code == LV_EVENT_GESTURE)
    {
        const auto dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        if (dir == LV_DIR_LEFT)
        {
            const auto index = static_cast<int8_t>(get_sensor_id_index()) + 1;
            inter_screen_interface_.show_sensor_detail_screen(
                index > static_cast<int8_t>(sensor_id_index::last) ? sensor_id_index::first : static_cast<sensor_id_index>(index));
        }
        else if (dir == LV_DIR_RIGHT)
        {
            const auto index = static_cast<int8_t>(get_sensor_id_index()) - 1;
            inter_screen_interface_.show_sensor_detail_screen(
                index < static_cast<int8_t>(sensor_id_index::first) ? sensor_id_index::last : static_cast<sensor_id_index>(index));
        }
    }
}

ui_screen_with_sensor_panel::panel_and_label ui_sensor_detail_screen::create_panel(const char *label_text, lv_align_t align, lv_coord_t x_ofs,
                                                                                   lv_coord_t y_ofs, lv_coord_t w, lv_coord_t h)
{
    auto panel = lv_obj_create(screen_);
    lv_obj_set_size(panel, w, h);
    lv_obj_align(panel, align, x_ofs, y_ofs);
    lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(panel, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(panel, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_dither_mode(panel, LV_DITHER_ORDERED, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(panel, 15, LV_PART_MAIN | LV_STATE_DEFAULT);
    set_padding_zero(panel);

    auto current_static_label = create_a_label(panel, &all_14_font, LV_ALIGN_TOP_MID, 0, 3);
    lv_label_set_text_static(current_static_label, label_text);

    auto value_label = create_a_label(panel, &regular_numbers_40_font, LV_ALIGN_BOTTOM_MID, 0, -3);

    panel_and_label pair{panel, value_label};
    set_panel_label_color(pair, sensor_level::no_level);

    return pair;
}

void ui_sensor_detail_screen::chart_draw_event_cb(lv_event_t *e)
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
        lv_draw_mask_line_points_init(&line_mask_param, dsc->p1->x, dsc->p1->y, dsc->p2->x, dsc->p2->y, LV_DRAW_MASK_LINE_SIDE_BOTTOM);
        int16_t line_mask_id = lv_draw_mask_add(&line_mask_param, NULL);

        /*Add a fade effect: transparent bottom covering top*/
        lv_coord_t h = lv_obj_get_height(obj);
        lv_draw_mask_fade_param_t fade_mask_param;
        lv_draw_mask_fade_init(&fade_mask_param, &obj->coords, LV_OPA_COVER, obj->coords.y1 + h / 8, LV_OPA_TRANSP, obj->coords.y2);
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
    else if (dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_X && dsc->text)
    {
        if (sensor_detail_screen_chart_series_data.size())
        {
            const auto data_interval_seconds = (sensor_detail_screen_chart_series_data.size() * 60);
            const float interval = float(chart_total_x_ticks - 1 - dsc->value) / (chart_total_x_ticks - 1);
            // ESP_LOGI("total seconds :%d,  Series length: %d,  %f", data_interval_seconds, sensor_detail_screen_chart_series_data.size(),
            // interval);
            const auto seconds_ago = (data_interval_seconds * interval) + ((esp32::millis() / 1000) - sensor_detail_screen_chart_series_time);

            seconds_to_timestring(seconds_ago, dsc->text, dsc->text_length);
        }
        else
        {
            strncpy(dsc->text, "-", dsc->text_length);
        }
    }
    else if (dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_Y && dsc->text)
    {
        snprintf(dsc->text, dsc->text_length, "%ld.%ld", dsc->value / 10, dsc->value % 10);
    }
}

void ui_sensor_detail_screen::set_current_values(sensor_id_index index, float value)
{
    sensor_detail_screen_chart_series_time = esp32::millis() / 1000;

    auto &&sensor_info = ui_interface_instance_.get_sensor_detail_info(index);

    if (sensor_info.stat.has_value())
    {
        ESP_LOGD(UI_TAG, "Found stats for %.*s", get_sensor_name(index).size(), get_sensor_name(index).data());
        auto &&stats = sensor_info.stat.value();

        lv_chart_set_type(sensor_detail_screen_chart, LV_CHART_TYPE_LINE);

        set_value_in_panel(panel_and_labels_[label_and_unit_label_average_index], index, stats.mean);
        set_value_in_panel(panel_and_labels_[label_and_unit_label_min_index], index, stats.min);
        set_value_in_panel(panel_and_labels_[label_and_unit_label_max_index], index, stats.max);

        auto &&values = sensor_info.history;

        set_value_in_panel(panel_and_labels_[label_and_unit_label_current_index], index, values[values.size() - 1]);

        lv_chart_set_point_count(sensor_detail_screen_chart, values.size());
        const auto range = std::max((stats.max - stats.min) * 0.1f, 2.0f);
        lv_chart_set_range(sensor_detail_screen_chart, LV_CHART_AXIS_PRIMARY_Y, graph_multiplier * std::floor(stats.min - range / 2),
                           graph_multiplier * std::ceil(stats.max + range / 2));

        sensor_detail_screen_chart_series_data.resize(values.size());
        for (auto index = 0; index < sensor_detail_screen_chart_series_data.size(); index++)
        {
            sensor_detail_screen_chart_series_data[index] = std::lroundf(values[index] * graph_multiplier);
        }

        lv_chart_set_ext_y_array(sensor_detail_screen_chart, sensor_detail_screen_chart_series, sensor_detail_screen_chart_series_data.data());
    }
    else
    {
        ESP_LOGD(UI_TAG, "No stats for %.*s", get_sensor_name(index).size(), get_sensor_name(index).data());
        set_default_value_in_panel(panel_and_labels_[label_and_unit_label_average_index]);
        set_default_value_in_panel(panel_and_labels_[label_and_unit_label_min_index]);
        set_default_value_in_panel(panel_and_labels_[label_and_unit_label_max_index]);

        sensor_detail_screen_chart_series_data.clear();
        lv_chart_set_type(sensor_detail_screen_chart, LV_CHART_TYPE_NONE);
    }
}

void ui_sensor_detail_screen::seconds_to_timestring(int seconds, char *buffer, uint32_t buffer_len)
{
    if (seconds <= 1)
    {
        strncpy(buffer, "now", buffer_len);
    }
    else if (seconds < 60)
    {
        snprintf(buffer, buffer_len, "%d sec%s ago", seconds, seconds > 1 ? "s" : "");
    }
    else if (seconds < 60 * 60)
    {
        const auto minutes = static_cast<int>(seconds / 60.0);
        snprintf(buffer, buffer_len, "%d min%s ago", minutes, minutes > 1 ? "s" : "");
    }
    else if (seconds < 60 * 60 * 24)
    {
        const auto hours = static_cast<int>(seconds / (60.0 * 60.0));
        snprintf(buffer, buffer_len, "%d hour%s ago", hours, hours > 1 ? "s" : "");
    }
    else
    {
        const auto days = static_cast<int>(seconds / (60.0 * 60.0 * 24.0));
        snprintf(buffer, buffer_len, "%d day%s ago", days, days > 1 ? "s" : "");
    }
}
