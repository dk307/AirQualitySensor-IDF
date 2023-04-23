#pragma once

#include "ui/ui_screen.h"

class ui_launcher_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
     void show_screen();

  private:
    lv_style_t button_style_default_{};
    lv_style_t button_style_pressed_{};
    lv_obj_t *win_confirm_{};
    lv_obj_t *win_confirm_label_{};

    void screen_callback(lv_event_t *e);
    lv_obj_t *create_button(lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs, const lv_img_dsc_t *src, const char *label_str,
                            lv_event_cb_t event_cb);
    void init_confirm_win();
    void show_information(lv_event_t *e);
    void hardware_info(lv_event_t *e);
    void homekit_info(lv_event_t *e);
    void wifi_setup(lv_event_t *e);
    void restart(lv_event_t *);
    void factory_reset(lv_event_t *);
    void yes_win_confirm(lv_event_t *e);
    void close_win_confirm(lv_event_t *e);

    enum class confirm_type
    {
        restart,
        factory_reset
    };

    void show_confirm(confirm_type type);
};