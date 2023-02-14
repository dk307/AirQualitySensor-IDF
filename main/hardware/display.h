#pragma once

#include "lgfx_device.h"
// #include <semaphore_lockable.h>
// #include "ui\ui2.h"
// #include "ui\ui_interface.h"

class display
{
public:
    // display(ui_interface &ui_interface_) : ui_instance(ui_interface_)
    // {
    // }

    bool pre_begin();
    // void begin();
    // void loop();

    // void update_boot_message(const String &message);
    // void set_main_screen();

    // uint8_t get_brightness();
    // void set_brightness(uint8_t value);

private:
    LGFX display_device;
    // esp32::semaphore lgvl_mutex;

    // lv_disp_draw_buf_t draw_buf{};
    // lv_disp_drv_t disp_drv{};
    // lv_indev_drv_t indev_drv{};
    // lv_disp_t *lv_display{};
    // lv_color_t *disp_draw_buf{};
    // lv_color_t *disp_draw_buf2{};
    // ui ui_instance;

    // static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    // static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
};