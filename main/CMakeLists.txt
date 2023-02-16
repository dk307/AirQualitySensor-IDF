idf_component_register(SRCS "main.cpp" 
                            "hardware/sdcard.cpp" 
                            "hardware/lgfx_device.cpp" 
                            "hardware/display.cpp" 
                            "hardware/hardware.cpp" 
                            "hardware/lvgl_fs/lvgl_fs_sd_card.c" 
                            "sensor/sensors_definitions.cpp" 
                            "ui/ui2.cpp" 
                            "ui/ui_img_logo.c" 
                            "config/config_manager.cpp"
                            "util/string_helper.cpp"

                    INCLUDE_DIRS "."
                    REQUIRES fatfs spi_flash LovyanGFX lvgl mbedtls)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-deprecated-enum-enum-conversion")
