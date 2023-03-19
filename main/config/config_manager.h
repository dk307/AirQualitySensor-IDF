#pragma once

#include "credentials.h"
#include "util/noncopyable.h"
#include "util/semaphore_lockable.h"
#include <ArduinoJson.h>
#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

class config_data : esp32::noncopyable
{
  public:
    config_data()
    {
        setDefaults();
    }

    void setDefaults()
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);

        const auto defaultUserIDPassword = "admin";
        host_name_ = "Air Quality Sensor";
        web_user_credentials_ = credentials(defaultUserIDPassword, defaultUserIDPassword);
        wifi_credentials_ = credentials();
        manual_screen_brightness_.reset();
        useFahrenheit = true;
    }

    std::string get_host_name() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return host_name_;
    }

    void set_host_name(const std::string &host_name)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        host_name_ = host_name;
    }

    std::optional<uint8_t> get_manual_screen_brightness() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return manual_screen_brightness_;
    }
    void set_manual_screen_brightness(const std::optional<uint8_t> &screen_brightness)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        manual_screen_brightness_ = screen_brightness;
    }

    credentials get_web_user_credentials() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return web_user_credentials_;
    }

    void set_web_user_credentials(const credentials &web_user_credentials)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        web_user_credentials_ = web_user_credentials;
    }

    void set_wifi_credentials(const credentials &wifi_credentials)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        wifi_credentials_ = wifi_credentials;
    }

    credentials get_wifi_credentials() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return wifi_credentials_;
    }

    bool is_use_fahrenheit() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return useFahrenheit;
    }
    void set_use_fahrenheit(bool use_fahrenheit)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        useFahrenheit = use_fahrenheit;
    }

  private:
    std::string host_name_;
    credentials web_user_credentials_;
    credentials wifi_credentials_;
    std::optional<uint8_t> manual_screen_brightness_;
    mutable esp32::semaphore data_mutex_;
    bool useFahrenheit;
};

class config : public esp32::noncopyable
{
  public:
    bool begin();
    void save();
    void reset();

    static void erase();
    static config instance;

    std::string get_all_config_as_json();

    // does not restore to memory, needs reboot
    bool restore_all_config_as_json(const std::vector<uint8_t> &json, const std::string &sha256);

    config_data data;

  private:
    config() = default;
    static std::string read_file(const char *file_name);
    size_t write_to_file(const char *fileName, const void *data, size_t _size);

    template <class T, class JDoc> bool deserialize_to_json(const T &data, JDoc &jsonDocument);

    static std::string sha256_hash(const std::string &data);

    void save_config();
};
