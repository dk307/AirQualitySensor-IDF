#pragma once

#include "ui_screen.h"
#include <esp_log.h>

class ui_information_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();

  private:
    lv_obj_t *tab_view_{};
    lv_obj_t *network_table_{};
    lv_obj_t *config_table_{};
    lv_obj_t *system_table_{};

    lv_timer_t *refresh_timer_{};

    constexpr static int tab_width = 64;

    void screen_callback(lv_event_t *e);
    static lv_obj_t *create_table(lv_obj_t *tab);
    void tab_view_callback(lv_event_t *e);
    void load_information(lv_timer_t *);
};