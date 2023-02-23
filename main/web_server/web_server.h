#pragma once

#include "sensor/sensor.h"
#include "util/async_web_server/http_server.h"
#include "util/async_web_server/http_request.h"

#include <vector>



class web_server : esp32::http_server
{
public:
    void begin() override;
    static web_server instance;

private:
    web_server()
        : esp32::http_server(80)
    {
    }

    template <const uint8_t data[], const auto len>
    void handle_array_page_with_auth(esp32::http_request *request);

    // handlers
    void handle_login(esp32::http_request *request);
    void handle_logout(esp32::http_request *request);
    void handle_web_login_update(esp32::http_request *request);
    void handle_other_settings_update(esp32::http_request *request);
    void handle_factory_reset(esp32::http_request *request);
    void handle_restart_device(esp32::http_request *request);

    void handle_firmware_upload(esp32::http_request *request);
    void restore_configuration_upload(esp32::http_request *request);

    // // ajax
    void handle_information_get(esp32::http_request *request);
    void handle_config_get(esp32::http_request *request);

    // // helpers
    bool is_authenticated(esp32::http_request *request);

    bool check_authenticated(esp32::http_request *request);
    void redirect_to_root(esp32::http_request *request);

    // void on_event_connect(AsyncEventSourceClient *client);
    // void on_logging_connect(AsyncEventSourceClient *client);
    // bool filter_events(esp32::http_request *request);

    // // fs ajax
    void handle_dir_list(esp32::http_request *request);
    void handle_dir_create(esp32::http_request *request);
    void handle_fs_download(esp32::http_request *request);
    void handle_fs_delete(esp32::http_request *request);
    void handle_fs_rename(esp32::http_request *request);
    void handle_file_upload(esp32::http_request *request);

    static const char *get_content_type(const std::string &extension);

    template <class Array, class K, class T>
    static void add_key_value_object(Array &array, const K &key, const T &value);

    static void log_and_send_error(const esp32::http_request *request, httpd_err_code_t code, const std::string &error);
    static void send_empty_200(const esp32::http_request *request);
    static std::string get_file_sha256(const char *filename);

    // void notify_sensor_change(sensor_id_index id);

    // void send_log_data(const std::string& c);
    // static void on_get_log_info(esp32::http_request *request);

    // esp32::AsyncWebServer http_server{80};
    // esp32::AsyncEventSource events{"/events"};
    // std::unique_ptr<std::vector<uint8_t>> restore_config_data;

    // esp32::AsyncEventSource logging{"/logs"};
};
