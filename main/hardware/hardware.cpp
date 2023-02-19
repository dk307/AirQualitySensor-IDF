#include "hardware/hardware.h"
#include "hardware/display.h"

// #include "hardware/sdcard.h"
#include "wifi/wifi_sta.h"
#include "wifi/wifi_manager.h"
#include "config/config_manager.h"
#include "hardware/display.h"
#include "logging/logging_tags.h"
#include "util/helper.h"

#include <memory>
#include <sstream>
#include <iomanip>
#include <esp_timer.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_netif_types.h>

hardware hardware::instance;

static const char *timezone_strings[5]{
    "USA Eastern",
    "USA Central",
    "USA Mountain time",
    "USA Arizona",
    "USA Pacific",
};

std::string hardware::get_up_time()
{
    const auto now = esp_timer_get_time() / (1000 * 1000);
    const uint8_t hour = now / 3600;
    const uint8_t mins = (now % 3600) / 60;
    const uint8_t sec = (now % 3600) % 60;

    return esp32::str_sprintf("%02d hours %02d mins %02d secs", hour, mins, sec);
}

void hardware::set_screen_brightness(uint8_t value)
{
    if (current_brightness_ != value)
    {
        ESP_LOGI(DISPLAY_TAG, "Setting display brightness to %d", value);
        display_instance_.set_brightness(std::max<uint8_t>(30, value));
        current_brightness_ = value;
    }
}

std::string get_heap_info_str(uint32_t caps)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, caps);
    return esp32::to_string_join(esp32::stringify_size(info.total_free_bytes), " free out of ", esp32::stringify_size(info.total_allocated_bytes + info.total_free_bytes));
}

ui_interface::information_table_type hardware::get_information_table(information_type type)
{
    switch (type)
    {
    case information_type::system:
        return {
            // {"Firmware Version", to_string(VERSION)},
            {"IDF Version", std::string(esp_get_idf_version())},
            //{"Chip", to_string(ESP.getChipModel(), "\nRev: ", ESP.getChipRevision(), "\nFlash: ", stringify_size(ESP.getFlashChipSize()))},
            {"Heap", get_heap_info_str(MALLOC_CAP_INTERNAL)},
            {"PsRam", get_heap_info_str(MALLOC_CAP_SPIRAM)},
            {"Uptime", get_up_time()},
            //{"SD Card Size", to_string(SD.cardSize() / (1024 * 1024), " MB")},
            {"Screen Brightness", esp32::to_string_join((display_instance_.get_brightness() * 100) / 256, " %")},
            // {"SHT31 sensor status", get_sht31_status()},
            // {"SPS30 sensor status", get_sps30_error_register_status()},
        };

    case information_type::network:
    {
        ui_interface::information_table_type table;

        table.push_back({"Mode", "STA Mode"});

        wifi_ap_record_t info;
        const auto result_info = esp_wifi_sta_get_ap_info(&info);
        if (result_info != ESP_OK)
        {
            table.push_back({"Error", esp32::to_string_join("failed to get info with error", result_info)});
        }
        else
        {
            // IP2STR()
            // table.push_back({"Hostname", WiFi.getHostname()});
            // table.push_back({"IP address(wifi)", WiFi.localIP().toString()});
            table.push_back({"Ssid", std::string(reinterpret_cast<char *>(&info.ssid[0]))});
            table.push_back({"RSSI", esp32::to_string_join(info.rssi, " db")});
            // table.push_back({"Gateway address", WiFi.gatewayIP().toString()});
            // table.push_back({"Subnet", WiFi.subnetMask().toString()});
            // table.push_back({"DNS", WiFi.dnsIP().toString()});
        }

        table.push_back({"Mac Address", wifi_sta::get_mac_address()});

        // auto local_time_now = ntp_time::instance.get_local_time();
        // if (local_time_now.has_value())
        // {
        //     char value[64];
        //     tm tm;
        //     gmtime_r(&local_time_now.value(), &tm);
        //     table.push_back({"Local time", asctime_r(&tm, value)});
        // }
        // else
        // {
        table.push_back({"Local time", "Not Synced"});
        // }

        return table;
    }
    case information_type::config:
        return {
            {"Hostname", config::instance.instance.data.get_host_name()},
            {"NTP server", config::instance.instance.data.get_ntp_server()},
            {"NTP server refresh interval", std::to_string(config::instance.instance.data.get_ntp_server_refresh_interval())},
            {"Time zone", timezone_strings[static_cast<size_t>(config::instance.instance.data.get_timezone())]},
            {"SSID", config::instance.instance.data.get_wifi_ssid()},
            {"Web user name", config::instance.instance.data.get_web_user_name()},
            {"Screen brightness (%)", std::to_string((100 * config::instance.instance.data.get_manual_screen_brightness().value_or(0)) / 256)},
        };
        break;
    }
    return {};
}

std::optional<sensor_value::value_type> hardware::get_sensor_value(sensor_id_index index) const
{
    auto &&sensor = get_sensor(index);
    return sensor.get_value();
}

sensor_history::sensor_history_snapshot hardware::get_sensor_detail_info(sensor_id_index index)
{
    return (*sensors_history)[static_cast<size_t>(index)].get_snapshot(sensor_history::reads_per_minute);
}

bool hardware::is_wifi_connected()
{
    return wifi_manager::instance.is_wifi_connected();
}

std::string hardware::get_wifi_status()
{
    return wifi_manager::instance.get_wifi_status();
}

bool hardware::clean_sps_30()
{
    // const auto sps_error = sps30_start_manual_fan_cleaning();
    // if (sps_error != NO_ERROR)
    // {
    //     ESP_LOGE(SENSOR_SPS30_TAG, "SPS30 manual clean up failed with :%d", sps_error);
    //     return false;
    // }
    // else
    // {
    //     ESP_LOGI(SENSOR_SPS30_TAG, "SPS30 manual cleanup started");
    //     return true;
    // }
    return true;
}

void hardware::pre_begin()
{
    sensors_history = esp32::psram::make_unique<std::array<sensor_history, total_sensors>>();

    display_instance_.pre_begin();

    // // Wire is already used by touch i2c
    // if (!Wire1.begin(SDAWire, SCLWire))
    // {
    //     ESP_LOGE(HARDWARE_TAG, "Failed to begin I2C interface");
    //     return false;
    // }

    // scan_i2c_bus();

    // if (!bh1750_sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1))
    // {
    //     ESP_LOGI(SENSOR_BH1750_TAG, "Failed to start BH 1750");
    // }
    // else
    // {
    //     ESP_LOGI(SENSOR_BH1750_TAG, "BH1750 Initialized");
    // }

    // if (!sht31_sensor.begin(sht31_i2c_address, &Wire1)) // Wire is already used by touch i2c
    // {
    //     ESP_LOGE(SENSOR_SHT31_TAG, "Failed to start SHT31");
    // }
    // else
    // {
    //     ESP_LOGI(SENSOR_SHT31_TAG, "SHT31 Initialized");
    //     sht31_sensor.heatOff();
    // }

    // const auto sps_error = sps30_probe();
    // if (sps_error == NO_ERROR)
    // {
    //     ESP_LOGI(SENSOR_SPS30_TAG, "SPS30 Found");

    //     const auto sps_error1 = sps30_start_measurement();
    //     if (sps_error1 != NO_ERROR)
    //     {
    //         ESP_LOGE(SENSOR_SPS30_TAG, "SPS30 start measurement failed with :%d", sps_error1);
    //     }
    // }
    // else
    // {
    //     ESP_LOGE(SENSOR_SPS30_TAG, "SPS30 Probe failed with :%d", sps_error);
    // }

    // set_auto_display_brightness();
}

// void hardware::set_sensor_value(sensor_id_index index, const std::optional<sensor_value::value_type> &value)
// {
//     const auto i = static_cast<size_t>(index);
//     if (value.has_value())
//     {
//         (*sensors_history)[i].add_value(value.value());
//         sensors[i].set_value(value.value());
//     }
//     else
//     {
//         ESP_LOGW(HARDWARE_TAG, "Got an invalid value for sensor:%s", get_sensor_name(index));
//         (*sensors_history)[i].clear();
//         sensors[i].set_invalid_value();
//     }
// }

void hardware::begin()
{
    display_instance_.begin();

    // sensor_refresh_task = std::make_unique<esp32::task>([this]
    //                                                     {
    //                                                         ESP_LOGI( HARDWARE_TAG, "Hardware task started on core:%d", xPortGetCoreID());
    //                                                         do
    //                                                         {
    //                                                             read_bh1750_sensor();
    //                                                             read_sht31_sensor();
    //                                                             read_sps30_sensor();
    //                                                             set_auto_display_brightness();
    //                                                             vTaskDelay(500);
    //                                                         } while(true); });

    // // start on core 0
    // sensor_refresh_task->spawn_arduino_other_core("sensor task", 4196);
}

// void hardware::read_bh1750_sensor()
// {
//     std::optional<float> lux;
//     if (bh1750_sensor.measurementReady(true))
//     {
//         ESP_LOGV(SENSOR_BH1750_TAG, "Reading BH1750 sensor");

//         lux = bh1750_sensor.readLightLevel();
//         light_sensor_values.add_value(lux.value());
//     }

//     const auto now = millis();
//     if (now - bh1750_sensor_last_read >= sensor_history::sensor_interval)
//     {
//         if (lux.has_value())
//         {
//             const auto value = round_value(lux.value());
//             ESP_LOGI(SENSOR_BH1750_TAG, "Setting new value:%d", value.value_or(std::numeric_limits<sensor_value::value_type>::max()));
//             set_sensor_value(sensor_id_index::light_intensity, value);
//             bh1750_sensor_last_read = now;
//         }
//         else
//         {
//             ESP_LOGE(SENSOR_BH1750_TAG, "Failed to read from BH1750");
//             set_sensor_value(sensor_id_index::light_intensity, std::nullopt);
//         }
//     }
// }

// void hardware::read_sht31_sensor()
// {
//     const auto now = millis();
//     if (now - sht31_sensor.lastRead() >= sensor_history::sensor_interval)
//     {
//         if (sht31_sensor.read(false))
//         {
//             const auto temp = round_value(sht31_sensor.getFahrenheit());
//             const auto hum = round_value(sht31_sensor.getHumidity());
//             ESP_LOGI(SENSOR_SHT31_TAG, "Setting SHT31 sensor values:%d F, %d %%",
//                      temp.value_or(std::numeric_limits<sensor_value::value_type>::max()),
//                      hum.value_or(std::numeric_limits<sensor_value::value_type>::max()));
//             set_sensor_value(sensor_id_index::temperatureF, temp);
//             set_sensor_value(sensor_id_index::humidity, hum);
//             sht31_last_error = SHT31_OK;
//         }
//         else
//         {
//             sht31_last_error = sht31_sensor.getError();
//             ESP_LOGE(SENSOR_SHT31_TAG, "Failed to read from SHT31 sensor with error:%x", sht31_last_error);
//             set_sensor_value(sensor_id_index::temperatureF, std::nullopt);
//             set_sensor_value(sensor_id_index::humidity, std::nullopt);
//         }
//     }
// }

// void hardware::read_sps30_sensor()
// {
//     const auto now = millis();
//     if (now - sps30_sensor_last_read >= sensor_history::sensor_interval)
//     {

//         bool read = false;
//         uint16_t ready = 0;

//         auto error = sps30_read_data_ready(&ready);

//         if ((error == NO_ERROR) && ready)
//         {
//             sps30_measurement m{};
//             error = sps30_read_measurement(&m);
//             if (error == NO_ERROR)
//             {
//                 sps30_sensor_last_read = now;
//                 read = true;

//                 const auto val_10 = round_value(m.mc_10p0);
//                 const auto val_1 = round_value(m.mc_1p0);
//                 const auto val_2_5 = round_value(m.mc_2p5);
//                 const auto val_4 = round_value(m.mc_4p0);
//                 const auto val_p = round_value(m.typical_particle_size);

//                 ESP_LOGI(SENSOR_SPS30_TAG, "Setting SPS30 sensor values PM2.5:%d, PM1:%d, PM4:%d, PM10:%d, Particle Size:%d",
//                          val_2_5.value_or(std::numeric_limits<sensor_value::value_type>::max()),
//                          val_1.value_or(std::numeric_limits<sensor_value::value_type>::max()),
//                          val_4.value_or(std::numeric_limits<sensor_value::value_type>::max()),
//                          val_10.value_or(std::numeric_limits<sensor_value::value_type>::max()),
//                          val_p.value_or(std::numeric_limits<sensor_value::value_type>::max()));

//                 set_sensor_value(sensor_id_index::pm_10, val_10);
//                 set_sensor_value(sensor_id_index::pm_1, val_1);
//                 set_sensor_value(sensor_id_index::pm_2_5, val_2_5);
//                 set_sensor_value(sensor_id_index::pm_4, val_4);
//                 set_sensor_value(sensor_id_index::typical_particle_size, val_p);
//             }
//             else
//             {
//                 ESP_LOGE(SENSOR_SPS30_TAG, "Failed to read from SPS sensor with failed to read measurement error:0x%x", error);
//             }
//         }
//         else
//         {
//             ESP_LOGE(SENSOR_SPS30_TAG, "Failed to read from SPS sensor with data not ready error:0x%x", error);
//         }

//         if (!read)
//         {
//             set_sensor_value(sensor_id_index::pm_10, std::nullopt);
//             set_sensor_value(sensor_id_index::pm_1, std::nullopt);
//             set_sensor_value(sensor_id_index::pm_2_5, std::nullopt);
//             set_sensor_value(sensor_id_index::pm_4, std::nullopt);
//             set_sensor_value(sensor_id_index::typical_particle_size, std::nullopt);
//         }
//     }
// }

std::optional<sensor_value::value_type> hardware::round_value(float val, int places)
{
    if (!isnan(val))
    {
        const auto expVal = places == 0 ? 1 : pow(10, places);
        const auto result = float(uint64_t(expVal * val + 0.5)) / expVal;
        return static_cast<sensor_value::value_type>(result);
    }

    return std::nullopt;
}

// void hardware::scan_i2c_bus()
// {
//     ESP_LOGD(HARDWARE_TAG, "Scanning...");

//     auto nDevices = 0;
//     for (auto address = 1; address < 127; address++)
//     {
//         // The i2c_scanner uses the return value of
//         // the Write.endTransmisstion to see if
//         // a device did acknowledge to the address.
//         Wire1.beginTransmission(address);
//         auto error = Wire1.endTransmission();

//         if (error == 0)
//         {
//             ESP_LOGI(HARDWARE_TAG, "I2C device found at address 0x%x", address);
//             nDevices++;
//         }
//     }
//     if (nDevices == 0)
//     {
//         ESP_LOGI(HARDWARE_TAG, "No I2C devices found");
//     }
// }

// std::string hardware::get_sht31_status()
// {
//     const auto status = sht31_sensor.readStatus();

//     if (status == 0xFFFF)
//     {
//         return "Not found.";
//     }

//     StreamString stream;
//     if (bitRead(status, 1))
//     {
//         stream.print("Checksum of last write transfer failed.");
//     }

//     if (bitRead(status, 2))
//     {
//         stream.print("Last command failed.");
//     }

//     // if (bitRead(status, 4))
//     // {
//     //     stream.print("System Reset Detected.");
//     // }

//     if (bitRead(status, 0x0d))
//     {
//         stream.print("Heater On.");
//     }

//     if (sht31_last_error != SHT31_OK)
//     {
//         stream.printf("Last Error:%x", sht31_last_error);
//     }

//     if (stream.isEmpty())
//     {
//         stream.print("Normal");
//     }

//     return stream;
// }

// std::string hardware::get_sps30_error_register_status()
// {
//     uint32_t device_status_flags{};
//     const auto error = sps30_read_device_status_register(&device_status_flags);

//     StreamString stream;
//     if (error != NO_ERROR)
//     {
//         stream.printf("Failed to read status with 0x%x", error);
//         return stream;
//     }

//     if (device_status_flags & SPS30_DEVICE_STATUS_FAN_ERROR_MASK)
//     {
//         stream.print("Fan Error.");
//     }
//     if (device_status_flags & SPS30_DEVICE_STATUS_LASER_ERROR_MASK)
//     {
//         stream.print("Laser Error.");
//     }
//     if (device_status_flags & SPS30_DEVICE_STATUS_FAN_SPEED_WARNING)
//     {
//         stream.print("Fan Speed Warning.");
//     }

//     if (stream.isEmpty())
//     {
//         stream.print("Normal");
//     }
//     return stream;
// }

// uint8_t hardware::lux_to_intensity(sensor_value::value_type lux)
// {
//     // https://learn.microsoft.com/en-us/windows/win32/sensorsapi/understanding-and-interpreting-lux-values
//     const auto intensity = (std::log10(lux) / 5) * 255;
//     return intensity;
// }

// void hardware::set_auto_display_brightness()
// {
//     const auto config_brightness = config::instance.data.get_manual_screen_brightness();

//     uint8_t required_brightness;
//     if (config_brightness.has_value())
//     {
//         required_brightness = config_brightness.value();
//     }
//     else
//     {
//         const auto avg_lux = light_sensor_values.get_average();
//         ESP_LOGV(SENSOR_BH1750_TAG, "Average lux:%d", avg_lux.value_or(128));
//         required_brightness = lux_to_intensity(avg_lux.value_or(128));
//     }

//     set_screen_brightness(required_brightness);
// }