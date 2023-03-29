#pragma once

#include "app_events.h"
#include "sensor/sensor_id.h"
#include "util/default_event.h"
#include "util/task_wrapper.h"
#include <hap.h>
#include <map>

class homekit_integration
{
  public:
    void begin();

    static homekit_integration instance;

    void forget_pairings();
    void reenable_pairing();

    bool is_paired();
    uint16_t get_connection_count();
    const std::string &get_password() const;
    const std::string &get_setup_id() const;

  private:
    homekit_integration() : homekit_task_([this] { homekit_task_ftn(); })
    {
    }

    esp32::default_event_subscriber instance_hap_event_{
        HAP_EVENT, ESP_EVENT_ANY_ID, [this](esp_event_base_t base, int32_t event, void *arg) { hap_event_handler(base, event, arg); }};

    esp32::default_event_subscriber instance_app_common_event_{
        APP_COMMON_EVENT, ESP_EVENT_ANY_ID, [this](esp_event_base_t base, int32_t event, void *data) { app_event_handler(base, event, data); }};

    esp32::task homekit_task_;
    hap_acc_t *accessory_{};
    std::string mac_address_;
    std::string name_;
    std::string setup_password_;
    std::string setup_id_;
    std::map<std::string_view, hap_serv_t *> services_;
    std::map<sensor_id_index, hap_char_t *> chars1_;
    std::map<hap_char_t *, sensor_id_index> chars2_;
    hap_char_t * air_quality_char{};

    void init_hap();
    void homekit_task_ftn();
    void create_sensor_services_and_chars();
    void hap_event_handler(esp_event_base_t event_base, int32_t event, void *data);
    void app_event_handler(esp_event_base_t base, int32_t event, void *data);
    void generate_password();
    static int sensor_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv);
    static float get_sensor_value(sensor_id_index index);
    static uint8_t get_air_quality();

    constexpr static uint32_t task_notify_restarting_bit = BIT(total_sensors + 1);
};