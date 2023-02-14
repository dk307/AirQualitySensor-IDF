#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
public:
    LGFX();

    const uint16_t Width = 320;
    const uint16_t Height = 480;

private:
    lgfx::Panel_ST7796 _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM _light_instance;
    lgfx::Touch_FT5x06 _touch_instance;
};
