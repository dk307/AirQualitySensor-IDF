idf_component_register(SRCS "main.cpp" "hardware/sdcard.cpp"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
