#pragma once

#include "app_events.h"
#include "lgfx_device.h"
#include "ui/ui2.h"
#include "ui/ui_interface.h"
#include "util/default_event.h"
#include "util/noncopyable.h"
#include "util/semaphore_lockable.h"
#include "util/task_wrapper.h"
#include <lvgl.h>

class display final : esp32::noncopyable
{
  public:
    display(ui_interface &ui_interface_) : lvgl_task_([this] { display::gui_task(); }), ui_instance_(ui_interface_)
    {
    }

    void start();

    uint8_t get_brightness();
    void set_brightness(uint8_t value);

  private:
    LGFX display_device_;
    esp32::task lvgl_task_;
    esp_timer_handle_t lv_periodic_timer_{nullptr};

    lv_disp_draw_buf_t draw_buf_{};
    lv_disp_drv_t disp_drv_{};
    lv_indev_drv_t indev_drv_{};
    lv_disp_t *lv_display_{};
    lv_color_t *disp_draw_buf_{};
    lv_color_t *disp_draw_buf2_{};
    ui ui_instance_;
    esp32::default_event_subscriber instance_app_common_event_{
        APP_COMMON_EVENT, ESP_EVENT_ANY_ID, [this](esp_event_base_t base, int32_t event, void *data) { app_event_handler(base, event, data); }};

    static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
    static void lv_tick_task(void *arg);
    void create_timer();
    void gui_task();
    void set_main_screen();
    void app_event_handler(esp_event_base_t, int32_t, void *);

    const int task_notify_wifi_changed_bit = BIT(total_sensors + 1);
    const int set_main_screen_changed_bit = BIT(total_sensors + 2);
    const int task_notify_restarting_bit = BIT(total_sensors + 3);
};