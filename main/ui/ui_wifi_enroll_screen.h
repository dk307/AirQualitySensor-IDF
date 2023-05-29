#pragma once

#include "ui/ui_screen.h"

class ui_wifi_enroll_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();

  private:
    void screen_callback(lv_event_t *e);

    static lv_obj_t *create_message_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);

    lv_obj_t *create_btn(const char *label_text, lv_event_cb_t event_cb);
    void stop_enrollment(lv_event_t *e);
};