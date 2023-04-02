#include "hardware/display/lgfx_device.h"
#include "hardware/pins.h"

LGFX::LGFX()
{
    {
        auto cfg = bus_instance_.config();

        cfg.port = 0;
        cfg.freq_write = 20000000;
        cfg.pin_wr = 47; // pin number connecting WR
        cfg.pin_rd = -1; // pin number connecting RD
        cfg.pin_rs = 0;  // Pin number connecting RS(D/C)
        cfg.pin_d0 = 9;  // pin number connecting D0
        cfg.pin_d1 = 46; // pin number connecting D1
        cfg.pin_d2 = 3;  // pin number connecting D2
        cfg.pin_d3 = 8;  // pin number connecting D3
        cfg.pin_d4 = 18; // pin number connecting D4
        cfg.pin_d5 = 17; // pin number connecting D5
        cfg.pin_d6 = 16; // pin number connecting D6
        cfg.pin_d7 = 15; // pin number connecting D7

        bus_instance_.config(cfg);              // Apply the settings to the bus.
        panel_instance_.setBus(&bus_instance_); // Sets the bus to the panel.
    }

    {                                        // Set display panel control.
        auto cfg = panel_instance_.config(); // Get the structure for display panel settings.

        cfg.pin_cs = -1;   // Pin number to which CS is connected (-1 = disable)
        cfg.pin_rst = 4;   // pin number where RST is connected (-1 = disable)
        cfg.pin_busy = -1; // pin number to which BUSY is connected (-1 = disable)

        // * The following setting values ​​are set to general default values ​​for each panel, and the pin number (-1 = disable) to which
        // BUSY is connected, so please try commenting out any unknown items.

        cfg.memory_width = LGFX::Width;   // Maximum width supported by driver IC
        cfg.memory_height = LGFX::Height; // Maximum height supported by driver IC
        cfg.panel_width = LGFX::Width;    // actual displayable width
        cfg.panel_height = LGFX::Height;  // actual displayable height
        cfg.offset_x = 0;                 // Panel offset in X direction
        cfg.offset_y = 0;                 // Panel offset in Y direction
        cfg.offset_rotation = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits = 1;
        cfg.readable = false;
        cfg.invert = true;
        cfg.rgb_order = false;
        cfg.dlen_16bit = false;
        cfg.bus_shared = true;

        panel_instance_.config(cfg);
    }

    {                                        // Set backlight control. (delete if not necessary)
        auto cfg = light_instance_.config(); // Get the structure for backlight configuration.

        cfg.pin_bl = 45;     // pin number to which the backlight is connected
        cfg.invert = false;  // true to invert backlight brightness
        cfg.freq = 44100;    // backlight PWM frequency
        cfg.pwm_channel = 0; // PWM channel number to use

        light_instance_.config(cfg);
        panel_instance_.setLight(&light_instance_); // Sets the backlight to the panel.
    }

    { // Configure settings for touch screen control. (delete if not necessary)
        auto cfg = touch_instance_.config();

        cfg.x_min = 0;   // Minimum X value (raw value) obtained from the touchscreen
        cfg.x_max = 319; // Maximum X value (raw value) obtained from the touchscreen
        cfg.y_min = 0;   // Minimum Y value obtained from touchscreen (raw value)
        cfg.y_max = 479; // Maximum Y value (raw value) obtained from the touchscreen
        cfg.pin_int = 7; // pin number to which INT is connected
        cfg.bus_shared = false;
        cfg.offset_rotation = 0;

        // For I2C connection
        cfg.i2c_port = 0;    // Select I2C to use (0 or 1)
        cfg.i2c_addr = 0x38; // I2C device address number
        cfg.pin_sda = 6;     // pin number where SDA is connected
        cfg.pin_scl = 5;     // pin number to which SCL is connected
        cfg.freq = 400000;   // set I2C clock

        touch_instance_.config(cfg);
        panel_instance_.setTouch(&touch_instance_); // Set the touchscreen to the panel.
    }

    setPanel(&panel_instance_); // Sets the panel to use.
}