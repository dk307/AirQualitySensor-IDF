#include "config_manager.h"
#include "app_events.h"
#include "logging/logging_tags.h"
#include "util/default_event.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/hash/hash.h"
#include "util/helper.h"
#include "util/psram_allocator.h"
#include <esp_log.h>
#include <filesystem>

constexpr std::string_view host_name_key{"host_name"};
constexpr std::string_view use_fahrenheit_key{"use_fahrenheit"};
constexpr std::string_view web_login_username_key{"web_username"};
constexpr std::string_view web_login_password_key{"web_password"};
constexpr std::string_view ssid_key{"ssid"};
constexpr std::string_view ssid_password_key{"ssid_password"};
constexpr std::string_view screen_brightness_key{"scrn_brightness"};

constexpr std::string_view default_host_name{"Air Quality Sensor"};
constexpr std::string_view default_user_id_and_password{"admin"};

static const char HostNameId[] = "hostname";
static const char WebUserNameId[] = "webusername";
static const char WebPasswordId[] = "webpassword";
static const char SsidId[] = "ssid";
static const char SsidPasswordId[] = "ssidpassword";
static const char ScreenBrightnessId[] = "screenbrightness";
static const char UsefahrenheitId[] = "usefahrenheit";

config config::instance;

void config::begin()
{
    ESP_LOGD(CONFIG_TAG, "Loading Configuration");

    nvs_storage.begin("nvs", "config");

    ESP_LOGI(CONFIG_TAG, "Hostname:%s", get_host_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Web user name:%s", get_web_user_credentials().get_user_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Web user password:%s", get_web_user_credentials().get_password().c_str());
    ESP_LOGI(CONFIG_TAG, "Wifi ssid:%s", get_wifi_credentials().get_user_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Wifi ssid password:%s", get_wifi_credentials().get_password().c_str());
    ESP_LOGI(CONFIG_TAG, "Manual screen brightness:%d", get_manual_screen_brightness().value_or(0));
    ESP_LOGI(CONFIG_TAG, "Use Fahrenheit:%s", is_use_fahrenheit() ? "Yes" : "No");
}

void config::save()
{
    ESP_LOGI(CONFIG_TAG, "config save");
    nvs_storage.commit();
    CHECK_THROW_ESP(esp32::event_post(APP_COMMON_EVENT, CONFIG_CHANGE));
}

std::string config::get_all_config_as_json()
{
    BasicJsonDocument<esp32::psram::json_allocator> json_document(2048);

    json_document[(HostNameId)] = get_host_name();
    const auto web_cred = get_web_user_credentials();
    json_document[(WebUserNameId)] = web_cred.get_user_name();
    json_document[(WebPasswordId)] = web_cred.get_password();

    const auto wifi_cred = get_wifi_credentials();
    json_document[(SsidId)] = wifi_cred.get_user_name();
    json_document[(SsidPasswordId)] = wifi_cred.get_password();

    const auto brightness = get_manual_screen_brightness();
    if (brightness.has_value())
    {
        json_document[ScreenBrightnessId] = brightness.value();
    }
    else
    {
        json_document[ScreenBrightnessId] = nullptr;
    }

    json_document[(UsefahrenheitId)] = is_use_fahrenheit();

    std::string json;
    serializeJson(json_document, json);

    return json;
}

bool config::is_use_fahrenheit()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return nvs_storage.get(use_fahrenheit_key, true);
}

void config::set_use_fahrenheit(bool use_fahrenheit)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(use_fahrenheit_key, use_fahrenheit);
}

std::string config::get_host_name()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return nvs_storage.get(host_name_key, default_host_name);
}

void config::set_host_name(const std::string &host_name)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(host_name_key, host_name);
}

credentials config::get_web_user_credentials()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return credentials(nvs_storage.get(web_login_username_key, default_user_id_and_password),
                       nvs_storage.get(web_login_password_key, default_user_id_and_password));
}

void config::set_web_user_credentials(const credentials &web_user_credentials)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(web_login_username_key, web_user_credentials.get_user_name());
    nvs_storage.save(web_login_password_key, web_user_credentials.get_password());
}

std::optional<uint8_t> config::get_manual_screen_brightness()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    const auto value = nvs_storage.get(screen_brightness_key, static_cast<uint8_t>(0));
    return value == 0 ? std::nullopt : std::optional<uint8_t>(value);
}
void config::set_manual_screen_brightness(const std::optional<uint8_t> &screen_brightness)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(screen_brightness_key, screen_brightness.value_or(0));
}

void config::set_wifi_credentials(const credentials &wifi_credentials)
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    nvs_storage.save(ssid_key, wifi_credentials.get_user_name());
    nvs_storage.save(ssid_password_key, wifi_credentials.get_password());
}

credentials config::get_wifi_credentials()
{
    std::lock_guard<esp32::semaphore> lock(data_mutex_);
    return credentials(nvs_storage.get(ssid_key, std::string_view()), nvs_storage.get(ssid_password_key, std::string_view()));
}
