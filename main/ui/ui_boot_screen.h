#pragma once

#include "ui/ui_screen.h"

class ui_boot_screen final : public ui_screen
{
  public:
    using ui_screen::ui_screen;
    void init() override;
    void show_screen();
};