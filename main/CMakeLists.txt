idf_component_register(SRCS "main.cpp" 
                            "hardware/sdcard.cpp" 
                            "hardware/lgfx_device.cpp" 
                            "hardware/display.cpp" 
                            "hardware/hardware.cpp" 

                    INCLUDE_DIRS "."
                    REQUIRES LovyanGFX fatfs spi_flash)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
