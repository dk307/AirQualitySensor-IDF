#pragma once

#include "sensor/sensor.h"
#include "util/async_web_server/http_server.h"

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


    // void server_routing();

    // // handler
    // static void handle_login(esp32::AsyncWebServerRequest *request);
    // static void handle_logout(esp32::AsyncWebServerRequest *request);
    // static void wifi_update(esp32::AsyncWebServerRequest *request);
    // static void web_login_update(esp32::AsyncWebServerRequest *request);
    // static void other_settings_update(esp32::AsyncWebServerRequest *request);
    // static void factory_reset(esp32::AsyncWebServerRequest *request);
    // static void restart_device(esp32::AsyncWebServerRequest *request);

    // static void firmware_update_upload(esp32::AsyncWebServerRequest *request,
    //                                    const std::string &filename,
    //                                    size_t index,
    //                                    uint8_t *data,
    //                                    size_t len,
    //                                    bool final);
    // static void reboot_on_upload_complete(esp32::AsyncWebServerRequest *request);

    // void restore_configuration_upload(esp32::AsyncWebServerRequest *request,
    //                                   const std::string &filename,
    //                                   size_t index,
    //                                   uint8_t *data,
    //                                   size_t len,
    //                                   bool final);

    // static void handle_early_update_disconnect();

    // // ajax
    // static void sensor_get(esp32::AsyncWebServerRequest *request);
    // static void wifi_get(esp32::AsyncWebServerRequest *request);
    // static void information_get(esp32::AsyncWebServerRequest *request);
    // static void config_get(esp32::AsyncWebServerRequest *request);

    // // helpers
    // static bool is_authenticated(esp32::AsyncWebServerRequest *request);
    // static bool manage_security(esp32::AsyncWebServerRequest *request);
    // static void handle_not_found(esp32::AsyncWebServerRequest *request);
    // static void handle_file_read(esp32::AsyncWebServerRequest *request);
    // static bool is_captive_portal_request(esp32::AsyncWebServerRequest *request);
    // static void redirect_to_root(esp32::AsyncWebServerRequest *request);
    // static void handle_error(esp32::AsyncWebServerRequest *request, const std::string &error, int code);
    // void on_event_connect(AsyncEventSourceClient *client);
    // void on_logging_connect(AsyncEventSourceClient *client);
    // bool filter_events(esp32::AsyncWebServerRequest *request);

    // // fs ajax
    // static void handle_dir_list(esp32::AsyncWebServerRequest *request);
    // static void handle_dir_create(esp32::AsyncWebServerRequest *request);
    // static void handle_fs_download(esp32::AsyncWebServerRequest *request);
    // static void handle_fs_delete(esp32::AsyncWebServerRequest *request);
    // static void handle_fs_rename(esp32::AsyncWebServerRequest *request);
    // void handle_file_upload(esp32::AsyncWebServerRequest *request,
    //                         const std::string &filename,
    //                         size_t index,
    //                         uint8_t *data,
    //                         size_t len,
    //                         bool final);
    // static void handle_file_upload_complete(esp32::AsyncWebServerRequest *request);
    // static const char *get_content_type(const std::string &filename);
    // static std::string join_path(const std::string &part1, const std::string &part2);
    // static std::string get_file_md5(const std::string &path);

    // static bool is_ip(const std::string &str);
    // static std::string to_std::string_ip(const IPAddress &ip);
    // template <class Array, class K, class T>
    // static void add_key_value_object(Array &array, const K &key, const T &value);
    // template <class V, class T>
    // static void add_to_json_doc(V &doc, T id, float value);
    // void notify_sensor_change(sensor_id_index id);

    // void send_log_data(const std::string& c);
    // static void on_get_log_info(esp32::AsyncWebServerRequest *request);

    // esp32::AsyncWebServer http_server{80};
    // esp32::AsyncEventSource events{"/events"};
    // std::unique_ptr<std::vector<uint8_t>> restore_config_data;

    // esp32::AsyncEventSource logging{"/logs"};
};
