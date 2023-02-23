#include "web_server.h"

#include <ArduinoJson.h>
#include <dirent.h>
#include <esp_log.h>
#include <mbedtls/md.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filesystem>

#include "config/config_manager.h"
#include "hardware/hardware.h"
#include "hardware/sdcard.h"
#include "logging/logging_tags.h"
#include "operations/operations.h"
#include "util/async_web_server/http_request.h"
#include "util/async_web_server/http_response.h"
#include "util/filesystem/file.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/finally.h"
#include "util/hash/hash.h"
#include "util/helper.h"
#include "util/ota.h"
#include "util/psram_allocator.h"
#include "web/include/debug.html.gz.h"
#include "web/include/fs.html.gz.h"
#include "web/include/index.html.gz.h"
#include "web/include/login.html.gz.h"

static const char json_media_type[] = "application/json";
static const char js_media_type[] = "text/javascript";
static const char html_media_type[] = "text/html";
static const char css_media_type[] = "text/css";
static const char png_media_type[] = "image/png";
static const char TextPlainMediaType[] = "text/plain";

static const char SettingsUrl[] = "/media/settings.png";
static constexpr char LogoutUrl[] = "/media/logout.png";

static const char MD5Header[] = "md5";
static const char CacheControlHeader[] = "Cache-Control";
static const char CookieHeader[] = "Cookie";
static const char AuthCookieName[] = "ESPSESSIONID=";

// Web url
static constexpr char logo_url[] = "/media/logo.png";
static constexpr char favicon_url[] = "/media/favicon.png";
static constexpr char all_js_url[] = "/js/s.js";
static constexpr char datatables_js_url[] = "/js/extra/datatables.min.js";
static constexpr char moment_js_url[] = "/js/extra/moment.min.js";
static constexpr char bootstrap_css_url[] = "/css/bootstrap.min.css";
static constexpr char datatable_css_url[] = "/css/datatables.min.css";
static constexpr char root_url[] = "/";
static constexpr char login_url[] = "/login.html";
static constexpr char index_url[] = "/index.html";
static constexpr char debug_url[] = "/debug.html";
static constexpr char fs_url[] = "/fs.html";

// sd card file paths
static constexpr char logo_file_path[] = "/sd/web/logo.png";
static constexpr char all_js_file_path[] = "/sd/web/s.js";
static constexpr char datatable_js_file_path[] = "/sd/web/extra/datatables.min.js";
static constexpr char moment_js_file_path[] = "/sd/web/extra/moment.min.js";
static constexpr char bootstrap_css_file_path[] = "/sd/web/bootstrap.min.css";
static constexpr char datatables_css_file_path[] = "/sd/web/datatables.min.css";

web_server web_server::instance;

std::string create_hash(const std::string &user, const std::string &password, const std::string &host)
{
    esp32::hash::hash<MBEDTLS_MD_SHA256> hasher;
    hasher.update(user);
    hasher.update(password);
    hasher.update(host);
    auto result = hasher.finish();
    return esp32::format_hex(result);
}

template <const uint8_t data[], const auto len> void web_server::handle_array_page_with_auth(esp32::http_request *request)
{
    if (!is_authenticated(request))
    {
        esp32::http_response response(request);
        response.redirect(login_url);
        return;
    }

    esp32::array_response response(request, data, len, true, html_media_type);
    response.send_response();
}

void web_server::begin()
{
    esp32::http_server::begin();

    ESP_LOGD(WEBSERVER_TAG, "Setting up web server routing");
    add_fs_file_handler<logo_file_path, png_media_type>(favicon_url);
    add_fs_file_handler<logo_file_path, png_media_type>(logo_url);

    add_fs_file_handler<all_js_file_path, js_media_type>(all_js_url);
    add_fs_file_handler<datatable_js_file_path, js_media_type>(datatables_js_url);
    add_fs_file_handler<moment_js_file_path, js_media_type>(moment_js_url);

    add_fs_file_handler<bootstrap_css_file_path, css_media_type>(bootstrap_css_url);
    add_fs_file_handler<datatables_css_file_path, css_media_type>(datatable_css_url);

    // static pages from flash
    add_array_handler<login_html_gz, login_html_gz_len, true, html_media_type>(login_url);

    // static pages from flash with auth
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<index_html_gz, index_html_gz_len>>(root_url, HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<index_html_gz, index_html_gz_len>>(index_url, HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<debug_html_gz, debug_html_gz_len>>(debug_url, HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_array_page_with_auth<fs_html_gz, fs_html_gz_len>>(fs_url, HTTP_GET);

    // not static pages

    ESP_LOGD(WEBSERVER_TAG, "Setup web server routing");

    add_handler_ftn<web_server, &web_server::handle_login>("/login.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_logout>("/logout.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_other_settings_update>("/othersettings.update.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_web_login_update>("/weblogin.update.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_restart_device>("/restart.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_factory_reset>("/factory.reset.handler", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_firmware_upload>("/firmware.update.handler", HTTP_POST);

    add_handler_ftn<web_server, &web_server::handle_information_get>("/api/information/get", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_config_get>("/api/config/get", HTTP_GET);

    // fs ajax
    add_handler_ftn<web_server, &web_server::handle_dir_list>("/fs/list", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_dir_create>("/fs/mkdir", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_fs_download>("/fs/download", HTTP_GET);
    add_handler_ftn<web_server, &web_server::handle_fs_rename>("/fs/rename", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_fs_delete>("/fs/delete", HTTP_POST);
    add_handler_ftn<web_server, &web_server::handle_file_upload>("/fs/upload", HTTP_POST);

    // events.onConnect(std::bind(&web_server::on_event_connec&t, this,
    // std::placeholders::_1));
    // events.setFilter(std::bind(&web_server::filter_events, this,
    // std::placeholders::_1));

    // logging.onConnect(std::bind(&web_server::on_logging_connect, this,
    // std::placeholders::_1));
    // logging.setFilter(std::bind(&web_server::filter_events, this,
    // std::placeholders::_1));

    // http_server.addHandler(&events);
    // http_server.addHandler(&logging);
    // http_server.begin();
    // server_routing();
    // ESP_LOGI(WEBSERVER_TAG, "WebServer Started");

    // for (auto i = 0; i < total_sensors; i++)
    // {
    // 	const auto id = static_cast<sensor_id_index>(i);
    // 	hardware::instance.get_sensor(id).add_callback([id, this]
    // 												   {
    // notify_sensor_change(id);
    // });
    // }
}

bool web_server::check_authenticated(esp32::http_request *request)
{
    if (!is_authenticated(request))
    {
        log_and_send_error(request, HTTPD_403_FORBIDDEN, "Auth Failed");
        return false;
    }
    return true;
}

// bool web_server::filter_events(esp32::http_request *request)
// {
// 	if (!is_authenticated(request))
// 	{
// 		ESP_LOGW(WEBSERVER_TAG, "Dropping events request");
// 		return false;
// 	}
// 	return true;
// }

// void web_server::server_routing()
// {
// 	// form calls
// 	http_server.on(("/login.handler"), HTTP_POST, handle_login);
// 	http_server.on(("/logout.handler"), HTTP_POST, handle_logout);
// 	http_server.on(("/wifiupdate.handler"), HTTP_POST, wifi_update);

// http_server.on(("/othersettings.update.handler"), HTTP_POST,
// other_settings_update); 	http_server.on(("/weblogin.update.handler"),
// HTTP_POST, web_login_update);

// 	// ajax form call
// 	http_server.on(("/factory.reset.handler"), HTTP_POST, factory_reset);
// 	http_server.on(("/firmware.update.handler"), HTTP_POST,
// reboot_on_upload_complete, firmware_update_upload);
// 	http_server.on(("/setting.restore.handler"), HTTP_POST,
// reboot_on_upload_complete,
// 				   std::bind(&web_server::restore_configuration_upload,
// this, 							 std::placeholders::_1, std::placeholders::_2,
// std::placeholders::_3, 							 std::placeholders::_4, std::placeholders::_5,
// std::placeholders::_6));

// 	http_server.on(("/restart.handler"), HTTP_POST, restart_device);

// 	// json ajax calls
// 	http_server.on(("/api/sensor/get"), HTTP_GET, sensor_get);
// 	http_server.on(("/api/wifi/get"), HTTP_GET, wifi_get);
// 	http_server.on(("/api/information/get"), HTTP_GET, information_get);
// 	http_server.on(("/api/config/get"), HTTP_GET, config_get);

// 	// fs ajax
// 	http_server.on("/fs/list", HTTP_GET, handle_dir_list);
// 	http_server.on("/fs/mkdir", HTTP_POST, handle_dir_create);
// 	http_server.on("/fs/download", HTTP_GET, handle_fs_download);
// 	http_server.on("/fs/upload", HTTP_POST, handle_file_upload_complete,
// 				   std::bind(&web_server::handle_file_upload,
// this, 							 std::placeholders::_1, std::placeholders::_2,
// std::placeholders::_3, 							 std::placeholders::_4, std::placeholders::_5,
// std::placeholders::_6)); http_server.on("/fs/delete", HTTP_POST, handle_fs_delete); 	http_server.on("/fs/rename", HTTP_POST, handle_fs_rename);
// http_server.onNotFound(handle_file_read);

// 	// log
// 	http_server.on("/api/log/webstart", HTTP_POST,
// [this](esp32::http_request *request)
// 				   {
// 			if
// (logger::instance.enable_web_logging(std::bind(&web_server::send_log_data,
// this, std::placeholders::_1))) { 					request->send(200); 			} else {
// 					request->send(500);
// 			} });

// 	http_server.on("/api/log/webstop", HTTP_POST, [this](esp32::http_request
// *request)
// 				   {
// 			logger::instance.disable_web_logging();
// 					request->send(200); });

// 	http_server.on("/api/log/sdstart", HTTP_POST, [this](esp32::http_request
// *request)
// 				   {
// 			if (logger::instance.enable_sd_logging()) {
// 					request->send(200);
// 			} else {
// 					request->send(500);
// 			} });

// 	http_server.on("/api/log/sdstop", HTTP_POST, [this](esp32::http_request
// *request)
// 				   {
// 			logger::instance.disable_sd_logging();
// 					request->send(200); });

// 	http_server.on("/api/log/info", HTTP_GET, on_get_log_info);
// }

// void web_server::on_event_connect(AsyncEventSourceClient *client)
// {
// 	if (client->lastId())
// 	{
// 		ESP_LOGI(WEBSERVER_TAG, "Events client reconnect");
// 	}
// 	else
// 	{
// 		ESP_LOGI(WEBSERVER_TAG, "Events client first time");

// 		// send all the events
// 		for (auto i = 0; i < total_sensors; i++)
// 		{
// 			notify_sensor_change(static_cast<sensor_id_index>(i));
// 		}
// 	}
// }

// void web_server::on_logging_connect(AsyncEventSourceClient *client)
// {
// 	if (client->lastId())
// 	{
// 		ESP_LOGI(WEBSERVER_TAG, "Logging client reconnect");
// 	}
// 	else
// 	{
// 		ESP_LOGI(WEBSERVER_TAG, "Logging client first time");
// 	}
// 	send_log_data("Start");
// }

// void web_server::wifi_get(esp32::http_request *request)
// {
// 	ESP_LOGI(WEBSERVER_TAG, "/api/wifi/get");
// 	if (!check_authenticated(request))
// 	{
// 		return;
// 	}

// 	auto response = new AsyncJsonResponse(false, 256);
// 	auto jsonBuffer = response->getRoot();

// 	jsonBuffer[("captivePortal")] =
// wifi_manager::instance.is_captive_portal(); 	jsonBuffer[("ssid")] =
// config::instance.data.get_wifi_ssid(); 	response->setLength();
// 	request->send(response);
// }

// void web_server::wifi_update(esp32::http_request *request)
// {
// 	const auto SsidParameter = ("ssid");
// 	const auto PasswordParameter = ("wifipassword");

// 	ESP_LOGI(WEBSERVER_TAG, "Wifi Update");

// 	if (!check_authenticated(request))
// 	{
// 		return;
// 	}

// 	if (request->hasArg(SsidParameter) &&
// request->hasArg(PasswordParameter))
// 	{
// 		wifi_manager::instance.set_new_wifi(request->arg(SsidParameter),
// request->arg(PasswordParameter)); 		redirect_to_root(request); 		return;
// 	}
// 	else
// 	{
// 		handle_error(request, ("Required parameters not provided"),
// 400);
// 	}
// }

template <class Array, class K, class T> void web_server::add_key_value_object(Array &array, const K &key, const T &value)
{
    auto j1 = array.createNestedObject();
    j1[("key")] = key;
    j1[("value")] = value;
}

void web_server::handle_information_get(esp32::http_request *request)
{
    ESP_LOGD(WEBSERVER_TAG, "/api/information/get");
    if (!check_authenticated(request))
    {
        return;
    }

    BasicJsonDocument<esp32::psram::json_allocator> json_document(1024);
    JsonArray arr = json_document.to<JsonArray>();

    const auto data = hardware::instance.get_information_table(ui_interface::information_type::system);

    for (auto &&[key, value] : data)
    {
        add_key_value_object(arr, key, value);
    }

    std::string data_str;
    data_str.reserve(1024);
    serializeJson(json_document, data_str);

    esp32::array_response::send_response(request, data_str, js_media_type);
}

void web_server::handle_config_get(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "/api/config/get");
    if (!check_authenticated(request))
    {
        return;
    }
    const auto json = config::instance.get_all_config_as_json();
    esp32::array_response::send_response(request, json, js_media_type);
}

// template <class V, class T>
// void web_server::add_to_json_doc(V &doc, T id, float value)
// {
// 	if (!isnan(value))
// 	{
// 		doc[id] = serialized(std::string(value, 2));
// 	}
// }

// void web_server::sensor_get(esp32::http_request *request)
// {
// 	ESP_LOGI(WEBSERVER_TAG, "/api/sensor/get");
// 	if (!check_authenticated(request))
// 	{
// 		return;
// 	}
// 	auto response = new AsyncJsonResponse(false, 256);
// 	auto doc = response->getRoot();
// 	response->setLength();
// 	request->send(response);
// }

// Check if header is present and correct
bool web_server::is_authenticated(esp32::http_request *request)
{
    ESP_LOGV(WEBSERVER_TAG, "Checking if authenticated");
    auto const cookie = request->get_header(CookieHeader);
    if (cookie.has_value())
    {
        ESP_LOGV(WEBSERVER_TAG, "Found cookie:%s", cookie->c_str());

        const std::string token =
            create_hash(config::instance.data.get_web_user_name(), config::instance.data.get_web_password(), request->client_ip_address());

        if (cookie->find_last_of(std::string(AuthCookieName) + token) != -1)
        {
            ESP_LOGV(WEBSERVER_TAG, "Authentication Successful");
            return true;
        }
    }
    ESP_LOGI(WEBSERVER_TAG, "Authentication Failed");
    return false;
}

void web_server::handle_login(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "Handle login");

    const auto arguments = request->get_form_url_encoded_arguments({"username", "password"});
    auto &&user_name = arguments[0];
    auto &&password = arguments[1];

    if (user_name.has_value() && password.has_value())
    {
        esp32::http_response response(request);
        const bool correct_credentials = esp32::str_equals_case_insensitive(user_name.value(), config::instance.data.get_web_user_name()) &&
                                         (password.value() == config::instance.data.get_web_password());

        if (correct_credentials)
        {
            ESP_LOGI(WEBSERVER_TAG, "User/Password correct");

            const std::string token = create_hash(user_name.value(), password.value(), request->client_ip_address());
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

void web_server::handle_logout(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "Disconnection");
    esp32::http_response response(request);
    response.add_header("Set-Cookie", "ESPSESSIONID=0");
    response.redirect("/login.html?msg=User disconnected");
}

void web_server::handle_web_login_update(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "web login Update");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request->get_form_url_encoded_arguments({"webUserName", "webPassword"});
    auto &&user_name = arguments[0];
    auto &&password = arguments[1];

    if (user_name.has_value() && password.has_value())
    {
        ESP_LOGI(WEBSERVER_TAG, "Updating web username/password");
        config::instance.data.set_web_password(user_name.value());
        config::instance.data.set_wifi_password(password.value());
        config::instance.save();
        redirect_to_root(request);
    }
    else
    {
        log_and_send_error(request, HTTPD_400_BAD_REQUEST, "Parameters not supplied for update");
    }
}

void web_server::handle_other_settings_update(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "config Update");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request->get_form_url_encoded_arguments(
        {"hostName", "ntpServer", "ntpServerRefreshInterval", "timezone", "autoScreenBrightness", "screenBrightness"});
    auto &&host_name = arguments[0];
    auto &&ntp_server = arguments[1];
    auto &&ntp_server_refresh_interval = arguments[2];
    auto &&timezone = arguments[3];
    auto &&auto_screen_brightness = arguments[4];
    auto &&screen_brightness = arguments[5];

    if (host_name.has_value())
    {
        config::instance.data.set_host_name(host_name.value());
    }

    if (ntp_server.has_value())
    {
        config::instance.data.set_ntp_server(ntp_server.value());
    }

    if (ntp_server_refresh_interval.has_value())
    {
        config::instance.data.set_ntp_server_refresh_interval(std::atoi(ntp_server_refresh_interval.value().c_str()));
    }

    if (timezone.has_value())
    {
        config::instance.data.set_timezone(static_cast<TimeZoneSupported>(std::atoi(timezone.value().c_str())));
    }

    if (!auto_screen_brightness.has_value())
    {
        const auto value = screen_brightness.value_or("128");
        config::instance.data.set_manual_screen_brightness(std::atoi(value.c_str()));
    }
    else
    {
        config::instance.data.set_manual_screen_brightness(std::nullopt);
    }

    config::instance.save();

    redirect_to_root(request);
}

void web_server::handle_restart_device(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "restart");

    if (!check_authenticated(request))
    {
        return;
    }

    send_empty_200(request);
    operations::instance.reboot();
}

void web_server::handle_factory_reset(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "factoryReset");

    if (!check_authenticated(request))
    {
        return;
    }
    send_empty_200(request);
    operations::instance.factory_reset();
}

// void web_server::reboot_on_upload_complete(esp32::http_request *request)
// {
// 	ESP_LOGI(WEBSERVER_TAG, "reboot");

// 	if (!check_authenticated(request))
// 	{
// 		return;
// 	}

// 	request->send(200);
// 	operations::instance.reboot();
// }

void web_server::redirect_to_root(esp32::http_request *request)
{
    esp32::http_response response(request);
    response.redirect("/");
}

void web_server::handle_firmware_upload(esp32::http_request *request)
{
    ESP_LOGD(WEBSERVER_TAG, "firmwareUpdateUpload");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto hash_arg = request->get_header("sha256");

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

    const auto result = request->read_body([&ota](const std::vector<uint8_t> &data) {
        ota.write(data.data(), data.size());
        return ESP_OK;
    });

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

// void web_server::restore_configuration_upload(esp32::http_request *request,
// 											  const std::string
// &filename, 											  size_t index,
// uint8_t *data, 											  size_t len,
// bool final)
// {
// 	ESP_LOGI(WEBSERVER_TAG, "restoreConfigurationUpload");

// 	if (!check_authenticated(request))
// 	{
// 		return;
// 	}

// 	std::string error;
// 	if (!index)
// 	{
// 		restore_config_data = std::make_unique<std::vector<uint8_t>>();
// 	}

// 	for (size_t i = 0; i < len; i++)
// 	{
// 		restore_config_data->push_back(data[i]);
// 	}

// 	if (final)
// 	{
// 		std::string md5;
// 		if (request->hasHeader((MD5Header)))
// 		{
// 			md5 = request->getHeader((MD5Header))->value();
// 		}

// 		ESP_LOGD(WEBSERVER_TAG, "Expected MD5:%s", md5.c_str());

// 		if (md5.length() != 32)
// 		{
// 			handle_error(request, ("MD5 parameter invalid. Check file
// exists."), 500); 			return;
// 		}

// 		if
// (!config::instance.restore_all_config_as_json(*web_server::instance.restore_config_data,
// md5))
// 		{
// 			handle_error(request, ("Restore Failed"), 500);
// 			return;
// 		}
// 	}
// }

// void web_server::handle_error(esp32::http_request *request, const std::string
// &message, int code)
// {
// 	if (!message.isEmpty())
// 	{
// 		ESP_LOGE(WEBSERVER_TAG, "%s", message.c_str());
// 	}
// 	AsyncWebServerResponse *response = request->beginResponse(code,
// TextPlainMediaType, message); 	response->addHeader((CacheControlHeader),
// ("no-cache, no-store, must-revalidate")); 	response->addHeader(("Pragma"),
// ("no-cache")); 	response->addHeader(("Expires"), ("-1"));
// 	request->send(response);
// }

// void web_server::handle_early_update_disconnect()
// {
// 	operations::instance.abort_update();
// }

// void web_server::notify_sensor_change(sensor_id_index id)
// {
// 	if (events.count())
// 	{
// 		const auto &sensor = hardware::instance.get_sensor(id);
// 		const auto value = sensor.get_value();
// 		const std::string value_str = value.has_value() ?
// std::string(value.value(), 10) : std::string("-");

// 		BasicJsonDocument<esp32::psram::json_allocator>
// json_document(128);

// 		auto &&definition = get_sensor_definition(id);
// 		json_document["value"] = value_str;
// 		json_document["unit"] = definition.get_unit();
// 		json_document["type"] = definition.get_name();
// 		json_document["level"] =
// definition.calculate_level(value.value_or(0));

// 		std::string json;
// 		serializeJson(json_document, json);
// 		events.send(json.c_str(), "sensor", millis());
// 	}
// }

void web_server::handle_dir_list(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/list");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request->get_url_arguments({"dir"});
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
        const auto message = esp32::str_sprintf("Failed to opendir :%s", path.c_str());
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

    std::string data_str;
    data_str.reserve(8196);
    serializeJson(json_document, data_str);

    esp32::array_response::send_response(request, data_str, js_media_type);
}

void web_server::handle_fs_download(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/download");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request->get_url_arguments({"path"});
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

void web_server::handle_fs_delete(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/delete");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request->get_form_url_encoded_arguments({"deleteFilePath"});
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
        const auto message = esp32::str_sprintf("Failed to delete %s", path.c_str());
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, message);
    }
    else
    {
        send_empty_200(request);
    }
}

void web_server::handle_dir_create(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/mkdir");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request->get_form_url_encoded_arguments({"dir"});
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
        const auto message = esp32::str_sprintf("Failed to create %s", path.c_str());
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, message);
    }
    else
    {
        send_empty_200(request);
    }
}

void web_server::handle_fs_rename(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/rename");
    if (!check_authenticated(request))
    {
        return;
    }

    const auto arguments = request->get_form_url_encoded_arguments({"oldPath", "newPath"});
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
        const auto message = esp32::str_sprintf("Failed to rename %s", old_path.c_str());
        log_and_send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, message.c_str());
    }
    else
    {
        send_empty_200(request);
    }
}

void web_server::handle_file_upload(esp32::http_request *request)
{
    ESP_LOGI(WEBSERVER_TAG, "/fs/upload");

    if (!check_authenticated(request))
    {
        return;
    }

    const auto upload_dir_arg = request->get_header("uploadDir");
    const auto hash_arg = request->get_header("sha256");
    const auto file_name_arg = request->get_header("X-File-Name");
    const auto last_modified_arg = request->get_header("X-Last-Modified");

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

        const auto result = request->read_body([&file, &temp_full_path](const std::vector<uint8_t> &data) {
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
    if (!esp32::str_equals_case_insensitive(file_hash, hash_arg.value()))
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
        const auto last_modified = esp32::parse_number<time_t>(last_modified_arg.value());
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

void web_server::log_and_send_error(const esp32::http_request *request, httpd_err_code_t code, const std::string &error)
{
    ESP_LOGW(WEBSERVER_TAG, "%s", error.c_str());
    esp32::http_response response(request);
    response.send_error(code, error.c_str());
}

void web_server::send_empty_200(const esp32::http_request *request)
{
    esp32::http_response response(request);
    response.send_empty_200();
}

// void web_server::send_log_data(const std::string &c)
// {
// 	if (logging.count())
// 	{
// 		logging.send(c.c_str(), "logs", millis());
// 	}
// }

// void web_server::on_get_log_info(esp32::http_request *request)
// {
// 	ESP_LOGI(WEBSERVER_TAG, "/api/log/info");

// 	BasicJsonDocument<esp32::psram::json_allocator> json_document(128);
// 	json_document["logLevel"] =
// logger::instance.get_general_logging_level();

// 	std::string json;
// 	serializeJson(json_document, json);
// 	request->send(200, JsonMediaType, json);
// }