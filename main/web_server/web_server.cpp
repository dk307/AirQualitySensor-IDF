#include "web_server.h"
#include "config/config_manager.h"
#include "generated/web/include/ansi_up.js.gz.h"
#include "generated/web/include/bootstrap.min.css.gz.h"
#include "generated/web/include/chartist.min.css.gz.h"
#include "generated/web/include/debug.html.gz.h"
#include "generated/web/include/index.html.gz.h"
#include "generated/web/include/login.html.gz.h"
#include "generated/web/include/logo.png.h"
#include "generated/web/include/s.js.gz.h"
#include "hardware/hardware.h"
#include "hardware/sd_card.h"
#include "logging/commands.h"
#include "logging/logger.h"
#include "logging/logging_tags.h"
#include "operations/operations.h"
#include "util/arduino_json_helper.h"
#include "util/async_web_server/http_request.h"
#include "util/async_web_server/http_response.h"
#include "util/filesystem/file.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/finally.h"
#include "util/hash/hash.h"
#include "util/helper.h"
#include "util/misc.h"
#include "util/ota.h"
#include "util/psram_allocator.h"
#include <dirent.h>
#include <esp_log.h>
#include <filesystem>
#include <homekit/homekit_integration.h>
#include <mbedtls/md.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
#include "generated/web/include/datatables.min.css.gz.h"
#include "generated/web/include/datatables.min.js.gz.h"
#include "generated/web/include/fs.html.gz.h"
#include "generated/web/include/moment.min.js.gz.h"
#endif

static const char json_media_type[] = "application/json";
static const char js_media_type[] = "text/javascript";
static const char html_media_type[] = "text/html";
static const char css_media_type[] = "text/css";
static const char png_media_type[] = "image/png";

static const char CookieHeader[] = "Cookie";
static const char AuthCookieName[] = "ESPSESSIONID=";

// Web url
static constexpr char logo_url[] = "/media/logo.png";
static constexpr char favicon_url[] = "/media/favicon.png";
static constexpr char all_js_url[] = "/js/s.js";
static constexpr char datatables_js_url[] = "/js/extra/datatables.min.js";
static constexpr char ansi_up_js_url[] = "/js/extra/ansi_up.js";
static constexpr char bootstrap_css_url[] = "/css/bootstrap.min.css";
static constexpr char chartist_css_url[] = "/css/chartist.min.css";
static constexpr char root_url[] = "/";
static constexpr char login_url[] = "/login.html";
static constexpr char index_url[] = "/index.html";
static constexpr char debug_url[] = "/debug.html";
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
static constexpr char moment_js_url[] = "/js/extra/moment.min.js";
static constexpr char datatable_css_url[] = "/css/datatables.min.css";
static constexpr char fs_url[] = "/fs.html";
#endif

std::string create_hash(const credentials &cred, const std::string &host)
{
    esp32::hash::hash<MBEDTLS_MD_SHA256> hasher;
    hasher.update(cred.get_user_name());
    hasher.update(cred.get_password());
    hasher.update(host);
    auto result = hasher.finish();
    return esp32::format_hex(result);
}

template <const uint8_t data[], const auto len, const char *sha256> void web_server::handle_array_page_with_auth(esp32::http_request &request)
{
    if (!is_authenticated(request))
    {
        esp32::http_response response(request);
        response.redirect(login_url);
        return;
    }

    esp32::array_response response(request, {data, len}, sha256, true, html_media_type);
    response.send_response();
}

void web_server::begin()
{
    esp32::http_server::begin();

    ESP_LOGD(WEBSERVER_TAG, "Setting up web server routing");

    // static pages from flash , no auth
    add_array_handler<logo_png, logo_png_len, logo_png_sha256, false, png_media_type>(favicon_url);
    add_array_handler<logo_png, logo_png_len, logo_png_sha256, false, png_media_type>(logo_url);
    add_array_handler<login_html_gz, login_html_gz_len, login_html_gz_sha256, true, html_media_type>(login_url);
    add_array_handler<ansi_up_js_gz, ansi_up_js_gz_len, ansi_up_js_gz_sha256, true, js_media_type>(ansi_up_js_url);
    add_array_handler<chartist_min_css_gz, chartist_min_css_gz_len, chartist_min_css_gz_sha256, true, css_media_type>(chartist_css_url);
    add_array_handler<bootstrap_min_css_gz, bootstrap_min_css_gz_len, bootstrap_min_css_gz_sha256, true, css_media_type>(bootstrap_css_url);
    add_array_handler<s_js_gz, s_js_gz_len, s_js_gz_sha256, true, js_media_type>(all_js_url);
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
    add_array_handler<datatables_min_css_gz, datatables_min_css_gz_len, datatables_min_css_gz_sha256, true, css_media_type>(datatable_css_url);
    add_array_handler<datatables_min_js_gz, datatables_min_js_gz_len, datatables_min_js_gz_sha256, true, js_media_type>(datatables_js_url);
    add_array_handler<moment_min_js_gz, moment_min_js_gz_len, moment_min_js_gz_sha256, true, js_media_type>(moment_js_url);
#endif

    // static pages from flash with auth
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<index_html_gz, index_html_gz_len, index_html_gz_sha256>>(root_url, HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<index_html_gz, index_html_gz_len, index_html_gz_sha256>>(index_url,
                                                                                                                                  HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<debug_html_gz, debug_html_gz_len, debug_html_gz_sha256>>(debug_url,
                                                                                                                                  HTTP_GET);
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<fs_html_gz, fs_html_gz_len, fs_html_gz_sha256>>(fs_url, HTTP_GET);
#endif

    // non static pages
    add_handler_ftn<web_server, &web_server::handle_login>("/login.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_logout>("/logout.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_other_settings_update>("/othersettings.update.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_web_login_update>("/weblogin.update.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_restart_device>("/restart.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_factory_reset>("/factory.reset.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_homekit_setting_reset>("/homekit.reset.handler", HTTP_POST);

    add_handler_ftn<web_server, &web_server::handle_firmware_upload>("/firmware.update.handler", HTTP_POST);

    add_handler_ftn<web_server, &web_server::handle_sensor_get>("/api/sensor/get", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_sensor_stats>("/api/sensor/history/get", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_information_get>("/api/information/get", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_config_get>("/api/config/get", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_homekit_info_get>("/api/homekit/get", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_homekit_enable_pairing>("/api/homekit/enablepairing", HTTP_POST);

#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
    // fs ajax
    add_handler_ftn<web_server, &web_server::handle_dir_list>("/fs/list", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_dir_create>("/fs/mkdir", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_fs_download>("/fs/download", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_fs_rename>("/fs/rename", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_fs_delete>("/fs/delete", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_file_upload>("/fs/upload", HTTP_POST);
#endif

    // event source
    add_handler_ftn<web_server, &web_server::handle_events>("/events", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_logging>("/logs", HTTP_GET);

    // log
    add_handler_ftn<web_server, &web_server::handle_web_logging_start>("/api/log/webstart", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_web_logging_stop>("/api/log/webstop", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_sd_card_logging_start>("/api/log/sdstart", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_sd_card_logging_stop>("/api/log/sdstop", HTTP_POST);
    add_handler_ftn<web_server, &web_server::on_set_logging_level>("/api/log/loglevel", HTTP_POST);
    add_handler_ftn<web_server, &web_server::on_run_command>("/api/log/run", HTTP_POST);

    instance_sensor_change_event_.subscribe();
}

bool web_server::check_authenticated(esp32::http_request &request)
{
    if (!is_authenticated(request))
    {
        log_and_send_error(request, HTTPD_403_FORBIDDEN, "Auth Failed");
        return false;
    }
    return true;
}

template <class Array, class K, class T> void web_server::add_key_value_object(Array &array, const K &key, const T &value)
{
    auto j1 = array.createNestedObject();
    j1[("key")] = key;
    j1[("value")] = value;
}

void web_server::handle_information_get(esp32::http_request &request)
{
    ESP_LOGD(WEBSERVER_TAG, "/api/information/get");
    if (!check_authenticated(request))
    {
        return;
    }

    send_table_response(request, ui_interface::information_type::system);
}

void web_server::handle_sensor_get(esp32::http_request &request)
{
    ESP_LOGD(WEBSERVER_TAG, "/api/sensor/get");
    if (!check_authenticated(request))
    {
        return;
    }

    BasicJsonDocument<esp32::psram::json_allocator> json_document(2048);
    JsonArray array = json_document.to<JsonArray>();

    for (auto i = 0; i < total_sensors; i++)
    {
        const auto id = static_cast<sensor_id_index>(i);
        const auto &sensor = ui_interface_.get_sensor(id);
        const auto value = sensor.get_value();
        auto obj = array.createNestedObject();

        auto &&definition = get_sensor_definition(id);
        obj["value"] = value;
        obj["id"] = static_cast<uint8_t>(id);
        obj["unit"] = definition.get_unit();
        obj["type"] = definition.get_name();
        obj["level"] = static_cast<uint64_t>(definition.calculate_level(value));
    }

    send_json_response(request, json_document);
}

void web_server::handle_sensor_stats(esp32::http_request &request)
{
    ESP_LOGD(WEBSERVER_TAG, "api/sensor/history/get");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_url_arguments({"id"});
    auto &&id_arg = arguments[0];

    auto id_arg_num = id_arg.has_value() ? esp32::string::parse_number<uint8_t>(id_arg.value()) : std::nullopt;

    if (!id_arg_num.has_value() || (id_arg_num.value() >= total_sensors))
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "sensor id not supplied or invalid");
        return;
    }

    const auto id = static_cast<sensor_id_index>(id_arg_num.value());
    const auto &sensor_detail_info = ui_interface_.get_sensor_detail_info(id);

    BasicJsonDocument<esp32::psram::json_allocator> json_document(8 * 1024);

    auto stats_json = json_document.createNestedObject("stats");

    if (sensor_detail_info.stat.has_value())
    {
        auto &&stats = sensor_detail_info.stat.value();
        stats_json["max"].set(stats.max);
        stats_json["min"].set(stats.min);
        stats_json["mean"].set(stats.mean);
    }
    else
    {
        stats_json["max"].set(nullptr);
        stats_json["min"].set(nullptr);
        stats_json["mean"].set(nullptr);
    }

    json_document["history"].set(sensor_detail_info.history);

    send_json_response(request, json_document);
}

void web_server::handle_config_get(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/config/get");
    if (!check_authenticated(request))
    {
        return;
    }
    const auto json = config_.get_all_config_as_json();
    esp32::array_response::send_response(request, json, js_media_type);
}

// Check if header is present and correct
bool web_server::is_authenticated(esp32::http_request &request)
{
    ESP_LOGV(WEBSERVER_TAG, "Checking if authenticated");
    auto const cookie = request.get_header(CookieHeader);
    if (cookie.has_value())
    {
        ESP_LOGV(WEBSERVER_TAG, "Found cookie:%s", cookie->c_str());

        const std::string token = create_hash(config_.get_web_user_credentials(), request.client_ip_address());

        if (cookie == (std::string(AuthCookieName) + token))
        {
            ESP_LOGV(WEBSERVER_TAG, "Authentication Successful");
            return true;
        }
    }
    ESP_LOGD(WEBSERVER_TAG, "Authentication Failed");
    return false;
}

void web_server::handle_login(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "Handle login");

    const auto arguments = request.get_form_url_encoded_arguments({"username", "password"});
    auto &&user_name = arguments[0];
    auto &&password = arguments[1];

    if (user_name.has_value() && password.has_value())
    {
        const auto current_credentials = config_.get_web_user_credentials();

        esp32::http_response response(request);
        const bool correct_credentials = esp32::string::equals_case_insensitive(user_name.value(), current_credentials.get_user_name()) &&
                                         (password.value() == current_credentials.get_password());

        if (correct_credentials)
        {
            ESP_LOGI(WEBSERVER_TAG, "User/Password correct");

            const std::string token = create_hash(current_credentials, request.client_ip_address());
            ESP_LOGD(WEBSERVER_TAG, "Token:%s", token.c_str());

            const auto cookie_header = std::string(AuthCookieName) + token;
            response.add_header("Set-Cookie", cookie_header.c_str());

            response.redirect("/");
            ESP_LOGI(WEBSERVER_TAG, "Log in Successful");
            return;
        }

        ESP_LOGW(WEBSERVER_TAG, "Log in Failed for %s", user_name.value().c_str());
        response.redirect("/login.html?msg=Wrong username/password! Try again");
        return;
    }
    else
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameters not supplied for login");
    }
}

void web_server::handle_logout(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "Disconnection");
    esp32::http_response response(request);
    response.add_header("Set-Cookie", "ESPSESSIONID=0");
    response.redirect("/login.html?msg=User disconnected");
}

void web_server::handle_web_login_update(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "web login Update");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_form_url_encoded_arguments({"webUserName", "webPassword"});
    auto &&user_name = arguments[0];
    auto &&password = arguments[1];

    if (user_name.has_value() && password.has_value())
    {
        ESP_LOGI(WEBSERVER_TAG, "Updating web username/password");
        config_.set_web_user_credentials(credentials(user_name.value(), password.value()));
        config_.save();
        redirect_to_root(request);
    }
    else
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameters not supplied for update");
    }
}

void web_server::handle_other_settings_update(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "config Update");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_form_url_encoded_arguments({"hostName", "autoScreenBrightness", "screenBrightness", "useFahrenheit"});
    auto &&host_name = arguments[0];
    auto &&auto_screen_brightness = arguments[1];
    auto &&screen_brightness = arguments[2];
    auto &&use_fahrenheit = arguments[3];

    if (host_name.has_value())
    {
        config_.set_host_name(host_name.value());
    }

    if (!auto_screen_brightness.has_value())
    {
        const auto value = screen_brightness.value_or("128");
        config_.set_manual_screen_brightness(std::atoi(value.c_str()));
    }
    else
    {
        config_.set_manual_screen_brightness(std::nullopt);
    }

    config_.set_use_fahrenheit(use_fahrenheit.has_value());

    config_.save();

    redirect_to_root(request);
}

void web_server::handle_restart_device(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "restart");

    if (!check_authenticated(request))
    {
        return;
    }

    send_empty_200(request);
    operations::instance.reboot();
}

void web_server::handle_factory_reset(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "factoryReset");

    if (!check_authenticated(request))
    {
        return;
    }
    send_empty_200(request);
    operations::instance.factory_reset();
}

void web_server::handle_homekit_setting_reset(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "reset homekit");

    if (!check_authenticated(request))
    {
        return;
    }

    ui_interface_.forget_homekit_pairings();
    send_empty_200(request);
}

void web_server::handle_homekit_enable_pairing(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "enable homekit repairing");

    if (!check_authenticated(request))
    {
        return;
    }

    ui_interface_.reenable_homekit_pairing();
    send_empty_200(request);
}

void web_server::redirect_to_root(esp32::http_request &request)
{
    esp32::http_response response(request);
    response.redirect("/");
}

void web_server::handle_firmware_upload(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "firmwareUpdateUpload");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto hash_arg = request.get_header("sha256");

    if (!hash_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Hash not supplied for firmware");
        return;
    }

    ESP_LOGD(WEBSERVER_TAG, "Expected firmware hash: %s", hash_arg.value().c_str());

    std::array<uint8_t, 32> hash_binary{};
    if (!esp32::parse_hex(hash_arg.value(), hash_binary.data(), hash_binary.size()))
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Hash supplied for firmware is not valid");
        return;
    }

    esp32::ota_updator ota(hash_binary);

    const auto result = request.read_body([&ota](const std::vector<uint8_t> &data) { return ota.write2(data.data(), data.size()); });

    if (result != ESP_OK)
    {
        ota.abort();
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read body and OTA");
        return;
    }

    ota.end();

    ESP_LOGI(WEBSERVER_TAG, "Firmware updated");
    send_empty_200(request);
    operations::instance.reboot();
}

void web_server::notify_sensor_change(sensor_id_index id)
{
    try
    {
        if (events.connection_count())
        {
            queue_work<web_server, sensor_id_index, &web_server::send_sensor_data>(id);
        }
    }
    catch (const std::exception &ex)
    {
        ESP_LOGW(WEBSERVER_TAG, "Failed to queue http event for %s with %s", get_sensor_name(id).data(), ex.what());
    }
}

void web_server::send_sensor_data(sensor_id_index id)
{
    ESP_LOGD(WEBSERVER_TAG, "Sending sensor info for %.*s", get_sensor_name(id).size(), get_sensor_name(id).data());
    const auto &sensor = ui_interface_.get_sensor(id);
    const auto value = sensor.get_value();

    BasicJsonDocument<esp32::psram::json_allocator> json_document(128);

    auto &&definition = get_sensor_definition(id);
    json_document["value"] = value;
    json_document["id"] = static_cast<uint8_t>(id);
    json_document["level"] = static_cast<uint64_t>(definition.calculate_level(value));

    esp32::psram::string json;
    serializeJson(json_document, json);
    events.try_send(json.c_str(), "sensor", esp32::millis(), 0);
}

#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
void web_server::handle_dir_list(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/list");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_url_arguments({"dir"});
    auto &&dir_arg = arguments[0];

    if (!dir_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameters not supplied for dir list");
        return;
    }

    const std::filesystem::path mount_path(sd_card::mount_point);
    const std::filesystem::path path = (mount_path / std::filesystem::path(dir_arg.value()).lexically_relative("/")).lexically_normal();

    ESP_LOGD(WEBSERVER_TAG, "Dir listing for %s", path.c_str());

    auto dir = opendir(path.c_str());
    if (!dir)
    {
        const auto message = esp32::string::sprintf("Failed to opendir :%s", path.c_str());
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, message);
        return;
    }
    else
    {
        ESP_LOGD(WEBSERVER_TAG, "Done opendir for %s", path.c_str());
    }

    BasicJsonDocument<esp32::psram::json_allocator> json_document(8196);
    auto array = json_document.createNestedArray("data");

    auto entry = readdir(dir);
    while (entry)
    {
        auto nested_entry = array.createNestedObject();
        const auto full_path = path / entry->d_name;
        nested_entry["path"] = (std::filesystem::path("/") / full_path.lexically_relative(mount_path)).generic_string();
        nested_entry["isDir"] = entry->d_type == DT_DIR;
        nested_entry["name"] = entry->d_name;

        struct stat entry_stat
        {
        };
        if (stat(full_path.c_str(), &entry_stat) == -1)
        {
            ESP_LOGW(WEBSERVER_TAG, "Failed to stat %s", full_path.c_str());
        }
        else
        {
            nested_entry["size"] = entry->d_type == DT_DIR ? 0 : entry_stat.st_size;
            nested_entry["lastModified"] = entry_stat.st_mtim.tv_sec;
        }
        entry = readdir(dir);
    }
    closedir(dir);

    send_json_response(request, json_document);
}

void web_server::handle_fs_download(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/download");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_url_arguments({"path"});
    auto &&path_arg = arguments[0];

    if (!path_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameter not supplied for path");
        return;
    }

    const std::filesystem::path mount_path(sd_card::mount_point);
    const std::filesystem::path path = (mount_path / std::filesystem::path(path_arg.value()).lexically_relative("/")).lexically_normal();

    ESP_LOGI(WEBSERVER_TAG, "Downloading %s", path.c_str());

    const auto extension = esp32::str_lower_case(path.extension());
    esp32::fs_card_file_response response(request, path.c_str(), get_content_type(extension), true);
    response.send_response(); // this does all the checks
}

void web_server::handle_fs_delete(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/delete");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_form_url_encoded_arguments({"deleteFilePath"});
    auto &&delete_path_arg = arguments[0];

    if (!delete_path_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameter not supplied for delete");
        return;
    }

    const std::filesystem::path mount_path(sd_card::mount_point);
    const std::filesystem::path path = (mount_path / std::filesystem::path(delete_path_arg.value()).lexically_relative("/")).lexically_normal();

    ESP_LOGI(WEBSERVER_TAG, "Deleting %s", path.c_str());

    esp32::filesystem::file_info file_info(path);

    bool success = false;
    if (file_info.exists())
    {
        if (file_info.is_directory())
        {
            success = esp32::filesystem::remove_directory(path);
        }
        else
        {
            success = esp32::filesystem::remove(path);
        }
    }

    if (!success)
    {
        const auto message = esp32::string::sprintf("Failed to delete %s", path.c_str());
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, message);
    }
    else
    {
        send_empty_200(request);
    }
}

void web_server::handle_dir_create(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/mkdir");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_form_url_encoded_arguments({"dir"});
    auto &&path_arg = arguments[0];

    if (!path_arg)
    {
        ESP_LOGW(WEBSERVER_TAG, "Parameter not supplied for dir");
        esp32::http_response response(request);
        response.send_error(HTTPD_400_BAD_REQUEST);
        return;
    }

    const std::filesystem::path mount_path(sd_card::mount_point);
    const std::filesystem::path path = (mount_path / std::filesystem::path(path_arg.value()).lexically_relative("/")).lexically_normal();

    ESP_LOGD(WEBSERVER_TAG, "mkdir for %s", path.c_str());

    if (!esp32::filesystem::create_directory(path))
    {
        const auto message = esp32::string::sprintf("Failed to create %s", path.c_str());
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, message);
    }
    else
    {
        send_empty_200(request);
    }
}

void web_server::handle_fs_rename(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/rename");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_form_url_encoded_arguments({"oldPath", "newPath"});
    auto &&old_path_arg = arguments[0];
    auto &&new_path_arg = arguments[1];

    if (!old_path_arg || !new_path_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameter not supplied for dir");
        return;
    }

    const std::filesystem::path mount_path(sd_card::mount_point);
    const std::filesystem::path old_path = (mount_path / std::filesystem::path(old_path_arg.value()).lexically_relative("/")).lexically_normal();
    const std::filesystem::path new_path = (mount_path / std::filesystem::path(new_path_arg.value()).lexically_relative("/")).lexically_normal();

    ESP_LOGI(WEBSERVER_TAG, "rename from %s to %s", old_path.c_str(), new_path.c_str());

    if (!esp32::filesystem::rename(old_path, new_path))
    {
        const auto message = esp32::string::sprintf("Failed to rename %s", old_path.c_str());
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, message.c_str());
    }
    else
    {
        send_empty_200(request);
    }
}

void web_server::handle_file_upload(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/upload");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto upload_dir_arg = request.get_header("uploadDir");
    const auto hash_arg = request.get_header("sha256");
    const auto file_name_arg = request.get_header("X-File-Name");
    const auto last_modified_arg = request.get_header("X-Last-Modified");

    if (!upload_dir_arg || !hash_arg || !file_name_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameters not supplied for upload");
        return;
    }

    const std::filesystem::path mount_path(sd_card::mount_point);
    const std::filesystem::path upload_file_name =
        (mount_path / std::filesystem::path(upload_dir_arg.value()).lexically_relative("/") / file_name_arg.value()).lexically_normal();

    const auto temp_full_path = upload_file_name.generic_string() + ".temp";
    ESP_LOGI(WEBSERVER_TAG, "Creating Temp File: %s", temp_full_path.c_str());

    auto _ = esp32::finally([&temp_full_path] { esp32::filesystem::remove(temp_full_path); });

    {
        auto file = esp32::filesystem::file(temp_full_path.c_str(), "w+");

        const auto result = request.read_body([&file, &temp_full_path](const std::vector<uint8_t> &data) {
            const auto bytesWritten = file.write(data.data(), 1, data.size());
            if (bytesWritten != data.size())
            {
                ESP_LOGW(WEBSERVER_TAG, "Failed to write data to file :%s", temp_full_path.c_str());
                return ESP_FAIL;
            }
            return ESP_OK;
        });

        if (result != ESP_OK)
        {
            log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read body and write to file");
            return;
        }
    }

    const auto file_hash = get_file_sha256(temp_full_path.c_str());

    ESP_LOGD(WEBSERVER_TAG, "Written file hash: %s", file_hash.c_str());
    ESP_LOGD(WEBSERVER_TAG, "Expected file hash: %s", hash_arg.value().c_str());

    // check hash is same as expected
    if (!esp32::string::equals_case_insensitive(file_hash, hash_arg.value()))
    {
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Written file hash does not match");
        return;
    }

    if (!esp32::filesystem::rename(temp_full_path, upload_file_name))
    {
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to rename temp file");
        return;
    }

    // try setting last modified time
    if (last_modified_arg.has_value())
    {
        const auto last_modified = esp32::string::parse_number<time_t>(last_modified_arg.value());
        if (last_modified.has_value())
        {
            if (!esp32::filesystem::set_last_modified_time(upload_file_name, last_modified.value()))
            {
                ESP_LOGW(WEBSERVER_TAG, "Failed to set last modified time for %s", upload_file_name.c_str());
            }
        }
        else
        {
            ESP_LOGW(WEBSERVER_TAG, "Invalid last modified time specfied for %s, value:%s", upload_file_name.c_str(),
                     last_modified_arg.value().c_str());
        }
    }

    ESP_LOGI(WEBSERVER_TAG, "File Uploaded: %s", upload_file_name.c_str());
    send_empty_200(request);
}
#endif

void web_server::handle_events(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/events");

    if (!check_authenticated(request))
    {
        return;
    }

    events.add_request(request);

    ESP_LOGI(WEBSERVER_TAG, "Events client first time");

    // send all the events
    for (auto i = 0; i < total_sensors; i++)
    {
        notify_sensor_change(static_cast<sensor_id_index>(i));
    }
}

void web_server::handle_logging(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/logging");

    if (!check_authenticated(request))
    {
        return;
    }

    logging.add_request(request);
    ESP_LOGI(WEBSERVER_TAG, "Logging first time");
}

void web_server::handle_web_logging_start(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/log/webstart");

    if (!check_authenticated(request))
    {
        return;
    }

    if (logger_.enable_web_logging([this](std::unique_ptr<std::string> log) { received_log_data(std::move(log)); }))
    {
        send_empty_200(request);
    }
    else
    {
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to enable web logging");
    }
}

void web_server::handle_web_logging_stop(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/log/webstop");

    if (!check_authenticated(request))
    {
        return;
    }

    logger_.disable_web_logging();
    send_empty_200(request);
}

void web_server::handle_sd_card_logging_start(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/log/sdstart");

    if (!check_authenticated(request))
    {
        return;
    }
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
    if (logger_.enable_sd_logging())
    {
        send_empty_200(request);
    }
    else
    {
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to enable sd card logging");
    }
#else
    log_and_send_error(request, HTTPD_505_VERSION_NOT_SUPPORTED, "Failed to enable sd card logging");
#endif
}

void web_server::handle_sd_card_logging_stop(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/log/sdstop");

    if (!check_authenticated(request))
    {
        return;
    }
#ifdef CONFIG_ENABLE_SD_CARD_SUPPORT
    logger_.disable_sd_logging();
    send_empty_200(request);
#else
    log_and_send_error(request, HTTPD_505_VERSION_NOT_SUPPORTED, "Failed to enable sd card logging");
#endif
}

std::string web_server::get_file_sha256(const char *filename)
{
    esp32::hash::hash<MBEDTLS_MD_SHA256> hasher;
    std::vector<uint8_t> buf;
    buf.resize(8196);

    auto file = esp32::filesystem::file(filename, "rb");
    size_t bytes_read;
    do
    {
        bytes_read = file.read(buf.data(), 1, buf.size());
        if (bytes_read > 0)
        {
            hasher.update(buf.data(), bytes_read);
        }
    } while (bytes_read == buf.size());

    const auto hash = hasher.finish();
    return esp32::format_hex(hash);
}

const char *web_server::get_content_type(const std::string &extension)
{
    if (extension == ".htm")
        return "text/html";
    else if (extension == ".html")
        return "text/html";
    else if (extension == ".css")
        return "text/css";
    else if (extension == ".js")
        return "application/javascript";
    else if (extension == ".json")
        return "application/json";
    else if (extension == ".png")
        return "image/png";
    else if (extension == ".gif")
        return "image/gif";
    else if (extension == ".jpg")
        return "image/jpeg";
    else if (extension == ".ico")
        return "image/x-icon";
    else if (extension == ".xml")
        return "text/xml";
    else if (extension == ".pdf")
        return "application/x-pdf";
    else if (extension == ".zip")
        return "application/x-zip";
    else if (extension == ".gz")
        return "application/x-gzip";

    return "text/plain";
}

void web_server::log_and_send_error(const esp32::http_request &request, httpd_err_code_t code, const std::string &error)
{
    ESP_LOGW(WEBSERVER_TAG, "%s", error.c_str());
    esp32::http_response response(request);
    response.send_error(code, error.c_str());
}

void web_server::send_empty_200(const esp32::http_request &request)
{
    esp32::http_response response(request);
    response.send_empty_200();
}

void web_server::received_log_data(std::unique_ptr<std::string> log)
{
    try
    {
        if (logging.connection_count())
        {
            queue_work<web_server, std::unique_ptr<std::string>, &web_server::send_log_data>(std::move(log));
        }
    }
    catch (...)
    {
        // ignore any error
    }
}

void web_server::send_log_data(std::unique_ptr<std::string> log)
{
    logging.try_send((*log).c_str(), "logs", esp32::millis(), 0);
}

void web_server::on_set_logging_level(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/log/loglevel");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_form_url_encoded_arguments({"tag", "level"});
    auto &&tag_arg = arguments[0];
    auto &&level_arg = arguments[1];

    if (!tag_arg || !level_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameters not supplied for log level set");
        return;
    }

    const auto log_level = esp32::string::parse_number<uint8_t>(level_arg.value());

    if (!log_level.has_value() || (log_level.value() > ESP_LOG_VERBOSE) || (log_level.value() < ESP_LOG_NONE))
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Log level not valid");
        return;
    }

    logger_.set_logging_level(tag_arg.value().c_str(), static_cast<esp_log_level_t>(log_level.value()));
    send_empty_200(request);
}

void web_server::on_run_command(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/log/run");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request.get_form_url_encoded_arguments({"command"});
    auto &&command_arg = arguments[0];

    if (!command_arg)
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Command not supplied");
        return;
    }

    run_command(command_arg.value());

    send_empty_200(request);
}

void web_server::handle_homekit_info_get(esp32::http_request &request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/homekit/get");

    if (!check_authenticated(request))
    {
        return;
    }

    send_table_response(request, ui_interface::information_type::homekit);
}

void web_server::send_table_response(esp32::http_request &request, ui_interface::information_type type)
{
    BasicJsonDocument<esp32::psram::json_allocator> json_document(2048);
    JsonArray arr = json_document.to<JsonArray>();

    const auto data = ui_interface_.get_information_table(type);

    for (auto &&[key, value] : data)
    {
        add_key_value_object(arr, key, value);
    }

    send_json_response(request, json_document);
}

void web_server::send_json_response(esp32::http_request &request, const BasicJsonDocument<esp32::psram::json_allocator> &json_document)
{
    if (json_document.overflowed())
    {
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Json document overflowed");
        return;
    }

    esp32::psram::string json;
    json.reserve(json_document.memoryUsage() * 2);
    serializeJson(json_document, json);
    esp32::array_response::send_response(request, json, js_media_type);
}