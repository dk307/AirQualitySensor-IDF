#pragma once

#include "ui/ui_screen.h"

class ui_homekit_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();

  private:
    lv_obj_t *win_confirm_{};
    lv_obj_t *win_confirm_label_{};
    lv_obj_t *homekit_table_{};
    lv_timer_t *refresh_timer_{};

    void screen_callback(lv_event_t *e);
    lv_obj_t *create_btn(const char *label_text, lv_event_cb_t event_cb);
    void init_status_win();
    void load_information(lv_timer_t *);
    void yes_win_confirm(lv_event_t *e);
    void close_win_confirm(lv_event_t *e);
    void forgot_homekit_pairing(lv_event_t *e);
    void reenable_homekit_pairing(lv_event_t *e);
};