#pragma once

#include <lvgl.h>
#include "lgfx_device.h"
#include "util/task_wrapper.h"
#include "util/semaphore_lockable.h"
         
// #include "ui\ui2.h"
// #include "ui\ui_interface.h"

class display
{
public:
    // display(ui_interface &ui_interface_) : ui_instance(ui_interface_)
    // {
    // }

    display() : lvgl_task(std::bind(&display::gui_task, this))
    {
    }

    bool pre_begin();
    // void begin();

    // void update_boot_message(const String &message);
    // void set_main_screen();

    uint8_t get_brightness();
    void set_brightness(uint8_t value);

private:
    LGFX display_device;
    esp32::semaphore lvgl_mutex;
    esp32::task lvgl_task;
    esp_timer_handle_t lv_periodic_timer{nullptr};

    lv_disp_draw_buf_t draw_buf{};
    lv_disp_drv_t disp_drv{};
    lv_indev_drv_t indev_drv{};
    lv_disp_t *lv_display{};
    lv_color_t *disp_draw_buf{};
    lv_color_t *disp_draw_buf2{};
    // ui ui_instance;

    static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
    static void lv_tick_task(void *arg);
    void create_timer();
    void gui_task();
};