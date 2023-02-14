idf_component_register(SRCS "main.cpp" 
                            "hardware/sdcard.cpp" 
                            "hardware/lgfx_device.cpp" 
                            "hardware/display.cpp" 
                            "hardware/hardware.cpp" 
                            "hardware/lvgl_fs/lvgl_fs_sd_card.c" 

                    INCLUDE_DIRS "."
                    REQUIRES fatfs spi_flash LovyanGFX lvgl)

#set_source_files_properties(components/lvgl/src/extra/libs/png/lv_png.c PROPERTIES COMPILE_FLAGS -Wno-format -Wincompatible-pointer-types) 