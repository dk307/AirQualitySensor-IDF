#pragma once

#include "hardware/sensors/sensor.h"
#include "hardware/sensors/sensor_id.h"
#include "util/psram_allocator.h"
#include "util/singleton.h"
#include "wifi/wifi_manager.h"
#include <string>
#include <vector>

class config;
class sd_card;
class hardware;
class homekit_integration;

class ui_interface : public esp32::singleton<ui_interface>
{
  public:
    enum class information_type
    {
        network,
        homekit,
        system,
        config
    };

    typedef std::vector<std::pair<std::string_view, std::string>, esp32::psram::allocator<std::pair<std::string_view, std::string>>>
        information_table_type;
    information_table_type get_information_table(information_type type);

    void set_screen_brightness(uint8_t value);
    const sensor_value &get_sensor(sensor_id_index index);
    float get_sensor_value(sensor_id_index index);
    sensor_history::sensor_history_snapshot get_sensor_detail_info(sensor_id_index index);
    wifi_status get_wifi_status();
    std::string get_sps30_error_register_status();

    void start_wifi_enrollment();
    void stop_wifi_enrollment();

    void forget_homekit_pairings();
    void reenable_homekit_pairing();

    bool clean_sps_30();

#ifdef CONFIG_SCD4x_SENSOR_ENABLE
    bool factory_reset_scd4x();
#endif

    static std::string get_default_mac_address();
    static std::string get_version();
    static std::string get_reset_reason_string();
    static std::string get_chip_details();
    static std::string get_heap_info_str(uint32_t caps);
    static std::string get_up_time();

    void update(config &config,
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
                sd_card &sd_card,
#endif
                hardware &hardware, wifi_manager &wifi_manager, homekit_integration &homekit_integration)
    {
        config_ = &config;
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
        sd_card_ = &sd_card;
#endif
        hardware_ = &hardware;
        wifi_manager_ = &wifi_manager;
        homekit_integration_ = &homekit_integration;
    }

  private:
    ui_interface() = default;

    friend class esp32::singleton<ui_interface>;

    config *config_{};
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
    sd_card *sd_card_{};
#endif
    hardware *hardware_{};
    wifi_manager *wifi_manager_{};
    homekit_integration *homekit_integration_{};

    // info
    static void get_nw_info(ui_interface::information_table_type &table);
};