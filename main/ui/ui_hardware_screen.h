#pragma once

#include "ui/ui_screen.h"

class ui_hardware_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();

  private:
    lv_obj_t *win_status_{};
    lv_obj_t *win_status_label_{};

    void screen_callback(lv_event_t *e);
    lv_obj_t *create_btn(const char *label_text, lv_event_cb_t event_cb);
    void init_status_win();
    void close_win_confirm(lv_event_t *e);
    void clean_sps_30(lv_event_t *e);
};