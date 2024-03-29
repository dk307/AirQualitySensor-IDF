# Air Quality Sensor
Air quality sensor on ESP32-S3 using ESP-IDF framework

## Build State
[![Build](https://github.com/dk307/AirQualitySensor-IDF/actions/workflows/build.yml/badge.svg)](https://github.com/dk307/AirQualitySensor-IDF/actions/workflows/build.yml)

## Hardware Components
* WT32-SC01 Plus - ESP32S3 with 3.5 Touch screen
* SHT31 - Temperature Sensor (Optional)
* BH1750 - Light Sensor
* SPS30 Sensirion - Particle sensor
* SCD30 Sensirion - CO2 Sensor (Optional)
* SCD4x Sensirion - CO2 Sensor (Optional)

## Features
* LVGL based display UI
  * Touch screen UI
  * Graph showing last 6 hours of values
  * Wifi enroll using ESP Smart Config
  * Display brightness changes based on ambient light
  * Day & Night mode for display
    
* Web server
  * Responsive UI based on Bootstrap framework   
  * Real time sensor values using event streem
  * Graph showing last 6 hours of values
  * Firmware upgrade
  * SD card file manager
  * Debug page for remote debugging
* Homekit enabled

## Screenshots
![Login](./asserts/Login.gif)

![Features](./asserts/Features.gif)

