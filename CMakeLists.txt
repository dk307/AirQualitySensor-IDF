cmake_minimum_required(VERSION 3.5)

set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")

# Sensor 1
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.sensor1")

# Sensor 2
#set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.sensor2")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

#LVGL custom config file setup
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-I../main" APPEND)

set(ESP_IDF_LIB_PATH ${CMAKE_CURRENT_LIST_DIR}/components/extra/esp-idf-lib/components)
set(HOMEKIT_LIB_PATH ${CMAKE_CURRENT_LIST_DIR}/components/extra/esp-homekit-sdk/components/homekit)
set(EXTRA_COMPONENT_DIRS ${ESP_IDF_LIB_PATH}/esp_idf_lib_helpers 
                         ${ESP_IDF_LIB_PATH}/i2cdev 
                         ${ESP_IDF_LIB_PATH}/bh1750 
                         ${ESP_IDF_LIB_PATH}/sht3x 
                         ${ESP_IDF_LIB_PATH}/scd30 
                         ${ESP_IDF_LIB_PATH}/scd4x
                         ${HOMEKIT_LIB_PATH}/esp_hap_apple_profiles
                         ${HOMEKIT_LIB_PATH}/esp_hap_platform
                         ${HOMEKIT_LIB_PATH}/json_parser
                         ${HOMEKIT_LIB_PATH}/json_generator
                         ${HOMEKIT_LIB_PATH}/mu_srp
                         ${HOMEKIT_LIB_PATH}/hkdf-sha
                         ${HOMEKIT_LIB_PATH}/esp_hap_core)

project(air_quality_sensor)

# Enable colors for compile output
idf_build_set_property(COMPILE_OPTIONS "-fdiagnostics-color=always" APPEND)

message(STATUS "--------------Compile Info------------")
message(STATUS "IDF_PATH = ${IDF_PATH}")
message(STATUS "IDF_TARGET = ${IDF_TARGET}")
message(STATUS "PROJECT_NAME = ${PROJECT_NAME}")
message(STATUS "PROJECT_DIR = ${PROJECT_DIR}")
message(STATUS "BUILD_DIR = ${BUILD_DIR}")
message(STATUS "SDKCONFIG = ${SDKCONFIG}")
message(STATUS "SDKCONFIG_DEFAULTS = ${SDKCONFIG_DEFAULTS}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
message(STATUS "COMPILE_OPTIONS = ${COMPILE_OPTIONS}")
message(STATUS "---------------------------------------")
