#pragma once

#include "app_events.h"
#include "sensor/sensor.h"
#include "ui/ui_interface.h"
#include "util/async_web_server/http_event_source.h"
#include "util/async_web_server/http_request.h"
#include "util/async_web_server/http_server.h"
#include "util/default_event.h"
#include <vector>

class web_server final : esp32::http_server
{
  public:
    void begin() override;
    static web_server instance;

  private:
    web_server() : esp32::http_server(80)
    {
    }

    template <const uint8_t data[], const auto len, const char *sha256> void handle_array_page_with_auth(esp32::http_request &request);

    // handlers
    void handle_login(esp32::http_request &request);
    void handle_logout(esp32::http_request &request);
    void handle_web_login_update(esp32::http_request &request);
    void handle_other_settings_update(esp32::http_request &request);
    void handle_factory_reset(esp32::http_request &request);
    void handle_homekit_setting_reset(esp32::http_request &request);
    void handle_homekit_enable_pairing(esp32::http_request &request);
    void handle_restart_device(esp32::http_request &request);

    void handle_firmware_upload(esp32::http_request &request);


    // // ajax
    void handle_information_get(esp32::http_request &request);
    void handle_config_get(esp32::http_request &request);

    // // helpers
    bool is_authenticated(esp32::http_request &request);

    bool check_authenticated(esp32::http_request &request);
    void redirect_to_root(esp32::http_request &request);

    // fs ajax
    void handle_dir_list(esp32::http_request &request);
    void handle_dir_create(esp32::http_request &request);
    void handle_fs_download(esp32::http_request &request);
    void handle_fs_delete(esp32::http_request &request);
    void handle_fs_rename(esp32::http_request &request);
    void handle_file_upload(esp32::http_request &request);

    // events
    void handle_events(esp32::http_request &request);
    void handle_logging(esp32::http_request &request);

    static const char *get_content_type(const std::string &extension);

    template <class Array, class K, class T> static void add_key_value_object(Array &array, const K &key, const T &value);

    static void log_and_send_error(const esp32::http_request &request, httpd_err_code_t code, const std::string &error);
    static void send_empty_200(const esp32::http_request &request);
    static std::string get_file_sha256(const char *filename);

    void notify_sensor_change(sensor_id_index id);
    void send_sensor_data(sensor_id_index id);

    void received_log_data(std::unique_ptr<std::string> log);
    void send_log_data(std::unique_ptr<std::string> log);

    void handle_web_logging_start(esp32::http_request &request);
    void handle_web_logging_stop(esp32::http_request &request);
    void handle_sd_card_logging_start(esp32::http_request &request);
    void handle_sd_card_logging_stop(esp32::http_request &request);
    void on_set_logging_level(esp32::http_request &request);
    void on_run_command(esp32::http_request &request);

    void handle_homekit_info_get(esp32::http_request &request);

    void send_table_response(esp32::http_request &request, ui_interface::information_type type);

    esp32::event_source events;
    esp32::event_source logging;

    esp32::default_event_subscriber_typed<sensor_id_index> instance_sensor_change_event_{
        APP_COMMON_EVENT, SENSOR_VALUE_CHANGE, [this](esp_event_base_t, int32_t, sensor_id_index id) { notify_sensor_change(id); }};
};
