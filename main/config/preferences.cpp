#include "preferences.h"

void preferences::begin(const std::string_view &part_name, const std::string_view &namespace_name)
{
    if (started_)
    {
        CHECK_THROW_ESP2(ESP_FAIL, "Already started");
    }
    CHECK_THROW_ESP(nvs_flash_init_partition(part_name.data()));
    CHECK_THROW_ESP(nvs_open_from_partition(part_name.data(), namespace_name.data(), NVS_READWRITE, &handle_));
    started_ = true;
}

void preferences::end()
{
    if (started_)
    {
        nvs_close(handle_);
        started_ = false;
    }
}

void preferences::commit()
{
    CHECK_THROW_ESP(nvs_commit(handle_));
}

void preferences::save(const std::string_view &key, bool value)
{
    CHECK_THROW_ESP(nvs_set_u8(handle_, key.data(), value));
}

void preferences::save(const std::string_view &key, uint8_t value)
{
    CHECK_THROW_ESP(nvs_set_u8(handle_, key.data(), value));
}

bool preferences::get(const std::string_view &key, bool default_value)
{
    const uint8_t value = get(key, static_cast<uint8_t>(default_value));
    return value != 0;
}

uint8_t preferences::get(const std::string_view &key, uint8_t default_value)
{
    uint8_t value{};
    const auto err = nvs_get_u8(handle_, key.data(), &value);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return default_value;
    }
    CHECK_THROW_ESP(err);
    return value;
}

void preferences::save(const std::string_view &key, const std::string_view &value)
{
    CHECK_THROW_ESP(nvs_set_str(handle_, key.data(), value.data()));
}

void preferences::save(const std::string_view &key, const std::string &value)
{
    CHECK_THROW_ESP(nvs_set_str(handle_, key.data(), value.data()));
}

std::string preferences::get(const std::string_view &key, const std::string_view &default_value)
{
    size_t length{};
    const auto err = nvs_get_str(handle_, key.data(), NULL, &length);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return std::string(default_value);
    }
    CHECK_THROW_ESP(err);
    std::string data;
    data.resize(length);
    CHECK_THROW_ESP(nvs_get_str(handle_, key.data(), data.data(), &length));
    data.resize(length - 1); // length includes zero ending
    return data;
}
