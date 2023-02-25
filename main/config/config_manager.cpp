#include "config_manager.h"
#include "logging/logging_tags.h"
#include "util/filesystem/file_info.h"
#include "util/filesystem/filesystem.h"
#include "util/hash/hash.h"
#include "util/helper.h"
#include "util/psram_allocator.h"

#include <esp_log.h>
#include <filesystem>

static const char ConfigFilePath[] = "/sd/config.json";
static const char ConfigChecksumFilePath[] = "/sd/config_checksum.json";
static const char HostNameId[] = "hostname";
static const char WebUserNameId[] = "webusername";
static const char WebPasswordId[] = "webpassword";
static const char SsidId[] = "ssid";
static const char SsidPasswordId[] = "ssidpassword";
static const char ScreenBrightnessId[] = "screenbrightness";

config config::instance;

size_t config::write_to_file(const char *file_name, const void *data, size_t size)
{
    auto file = fopen(file_name, "w");
    if (!file)
    {
        return 0;
    }

    const auto bytesWritten = fwrite(data, 1, size, file);
    fclose(file);
    return bytesWritten;
}

void config::erase()
{
    esp32::filesystem::remove(ConfigChecksumFilePath);
    esp32::filesystem::remove(ConfigFilePath);
}

bool config::pre_begin()
{
    ESP_LOGD(CONFIG_TAG, "Loading Configuration");
    const auto config_data = read_file(ConfigFilePath);

    ESP_LOGD(CONFIG_TAG, "Config File:[%s]", config_data.c_str());

    if (config_data.empty())
    {
        ESP_LOGW(CONFIG_TAG, "No stored config found");
        reset();
        return false;
    }

    // read checksum from file
    const auto read_checksum = read_file(ConfigChecksumFilePath);

    ESP_LOGD(CONFIG_TAG, "Checksum File:[%s]", read_checksum.c_str());

    const auto checksum = md5_hash(config_data);

    if (!esp32::str_equals_case_insensitive(checksum, read_checksum))
    {
        ESP_LOGE(CONFIG_TAG, "Config data checksum mismatch Expected from file:%s Expected:%s", read_checksum.c_str(), checksum.c_str());
        reset();
        return false;
    }
    else
    {
        ESP_LOGD(CONFIG_TAG, "Checksum file is correct");
    }

    BasicJsonDocument<esp32::psram::json_allocator> json_document(2048);
    if (!deserialize_to_json(config_data.c_str(), json_document))
    {
        ESP_LOGE(CONFIG_TAG, "Config data  is not a valid json");
        reset();
        return false;
    }

    data.set_host_name(json_document[(HostNameId)].as<std::string>());
    data.set_web_user_name(json_document[(WebUserNameId)].as<std::string>());
    data.set_web_password(json_document[(WebPasswordId)].as<std::string>());
    data.set_wifi_ssid(json_document[(SsidId)].as<std::string>());
    data.set_wifi_password(json_document[(SsidPasswordId)].as<std::string>());

    const auto screen_brightness = json_document[ScreenBrightnessId];
    data.set_manual_screen_brightness(!screen_brightness.isNull() ? std::optional<uint8_t>(screen_brightness.as<uint8_t>()) : std::nullopt);

    ESP_LOGI(CONFIG_TAG, "Loaded Config from file");

    ESP_LOGI(CONFIG_TAG, "Hostname:%s", data.get_host_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Web user name:%s", data.get_web_user_name().c_str());
    ESP_LOGI(CONFIG_TAG, "Web user password:%s", data.get_web_password().c_str());
    ESP_LOGI(CONFIG_TAG, "Wifi ssid:%s", data.get_wifi_ssid().c_str());
    ESP_LOGI(CONFIG_TAG, "Wifi ssid password:%s", data.get_wifi_password().c_str());
    ESP_LOGI(CONFIG_TAG, "Manual screen brightness:%d", data.get_manual_screen_brightness().value_or(0));

    return true;
}

void config::reset()
{
    ESP_LOGI(CONFIG_TAG, "config reset is requested");
    data.setDefaults();
    save_config();
}

void config::save()
{
    ESP_LOGD(CONFIG_TAG, "config save is requested");
    save_config();
}

void config::save_config()
{
    ESP_LOGI(CONFIG_TAG, "Saving configuration");

    BasicJsonDocument<esp32::psram::json_allocator> json_document(2048);

    json_document[(HostNameId)] = data.get_host_name();
    json_document[(WebUserNameId)] = data.get_web_user_name();
    json_document[(WebPasswordId)] = data.get_web_password();
    json_document[(SsidId)] = data.get_wifi_ssid();
    json_document[(SsidPasswordId)] = data.get_wifi_password();

    const auto brightness = data.get_manual_screen_brightness();
    if (brightness.has_value())
    {
        json_document[ScreenBrightnessId] = brightness.value();
    }
    else
    {
        json_document[ScreenBrightnessId] = nullptr;
    }

    std::string json;
    serializeJson(json_document, json);

    if (write_to_file((ConfigFilePath), json.c_str(), json.length()) == json.length())
    {
        const auto checksum_value = esp32::hash::md5(json);
        const auto checksum = esp32::format_hex(checksum_value.data(), checksum_value.size());

        if (write_to_file((ConfigChecksumFilePath), checksum.c_str(), checksum.length()) != checksum.length())
        {
            ESP_LOGE(CONFIG_TAG, "Failed to write config checksum file");
        }
    }
    else
    {
        ESP_LOGE(CONFIG_TAG, "Failed to write config file");
    }

    ESP_LOGI(CONFIG_TAG, "Saving Configuration done");
    call_change_listeners();
}

std::string config::read_file(const char *file_name)
{
    esp32::filesystem::file_info file_info(file_name);

    if (file_info.exists())
    {
        const auto size = file_info.size();
        auto file = fopen(file_name, "r");
        if (file)
        {
            std::string json;
            json.resize(size);

            if (fread(json.data(), 1, size, file) == size)
            {
                fclose(file);
                return json;
            }
            else
            {
                fclose(file);
            }
        }
    }
    else
    {
        ESP_LOGI(CONFIG_TAG, "File not found %s", file_name);
    }

    return std::string();
}

std::string config::get_all_config_as_json()
{
    return read_file((ConfigFilePath));
}

bool config::restore_all_config_as_json(const std::vector<uint8_t> &json, const std::string &hashMd5)
{
    BasicJsonDocument<esp32::psram::json_allocator> json_doc(2048);
    if (!deserialize_to_json(json, json_doc))
    {
        return false;
    }

    const auto checksum_value = esp32::hash::md5(json.data(), json.size());
    const auto expected_md5 = esp32::format_hex(checksum_value.data(), checksum_value.size());

    if (!esp32::str_equals_case_insensitive(expected_md5, hashMd5))
    {
        ESP_LOGE(CONFIG_TAG, "Uploaded Md5 for config does not match. File md5:%s", expected_md5.c_str());
        return false;
    }

    if (write_to_file((ConfigFilePath), json.data(), json.size()) != json.size())
    {
        return false;
    }

    if (write_to_file((ConfigChecksumFilePath), hashMd5.c_str(), hashMd5.length()) != hashMd5.length())
    {
        return false;
    }
    return true;
}

template <class T, class JDoc> bool config::deserialize_to_json(const T &data, JDoc &jsonDocument)
{
    DeserializationError error = deserializeJson(jsonDocument, data);

    // Test if parsing succeeds.
    if (error)
    {
        ESP_LOGE(CONFIG_TAG, "deserializeJson for config failed: %s", error.c_str());
        return false;
    }
    return true;
}

std::string config::md5_hash(const std::string &data)
{
    const auto checksum_value = esp32::hash::md5(data);
    return esp32::format_hex(checksum_value.data(), checksum_value.size());
}