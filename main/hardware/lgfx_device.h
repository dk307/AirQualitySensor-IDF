#pragma once

#include "util/noncopyable.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX final : public lgfx::LGFX_Device, esp32::noncopyable
{
  public:
    LGFX();

    const uint16_t Width = 320;
    const uint16_t Height = 480;

  private:
    lgfx::Panel_ST7796 panel_instance_;
    lgfx::Bus_Parallel8 bus_instance_;
    lgfx::Light_PWM light_instance_;
    lgfx::Touch_FT5x06 touch_instance_;
};
