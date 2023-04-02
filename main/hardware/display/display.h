#pragma once

#include "app_events.h"
#include "lgfx_device.h"
#include "ui/ui2.h"
#include "ui/ui_interface.h"
#include "util/default_event.h"
#include "util/semaphore_lockable.h"
#include "util/singleton.h"
#include "util/task_wrapper.h"
#include <lvgl.h>

class display final : public esp32::singleton<display>
{
  public:
    void begin();

    uint8_t get_screen_brightness();
    void set_screen_brightness(uint8_t value);

  private:
    display(config &config, ui_interface &ui_interface)
        : lvgl_task_([this] { display::gui_task(); }), ui_interface_(ui_interface), ui_instance_(config, ui_interface)
    {
    }

    friend class esp32::singleton<display>;

    LGFX display_device_;
    esp32::task lvgl_task_;
    ui_interface &ui_interface_;

    lv_disp_draw_buf_t draw_buf_{};
    lv_disp_drv_t disp_drv_{};
    lv_indev_drv_t indev_drv_{};
    lv_disp_t *lv_display_{};
    lv_color_t *disp_draw_buf_{};
    lv_color_t *disp_draw_buf2_{};
    ui ui_instance_;
    esp32::default_event_subscriber instance_app_common_event_{
        APP_COMMON_EVENT, ESP_EVENT_ANY_ID, [this](esp_event_base_t base, int32_t event, void *data) { app_event_handler(base, event, data); }};
    uint8_t current_brightness_{0};

    static void IRAM_ATTR display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void IRAM_ATTR touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
    void gui_task();
    void app_event_handler(esp_event_base_t, int32_t, void *);

    constexpr static uint32_t task_notify_wifi_changed_bit = BIT(total_sensors + 1);
    constexpr static uint32_t set_main_screen_changed_bit = BIT(total_sensors + 2);
    constexpr static uint32_t task_notify_restarting_bit = BIT(total_sensors + 3);
    constexpr static uint32_t config_changed_bit = BIT(total_sensors + 4);
    constexpr static uint32_t idenitfy_device_bit = BIT(total_sensors + 5);
};