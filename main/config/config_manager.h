#pragma once

#include "util/change_callback.h"
#include "util/semaphore_lockable.h"

#include <ArduinoJson.h>
#include <atomic>
#include <mutex>
#include <optional>

enum class TimeZoneSupported
{
    USEastern = 0,
    USCentral = 1,
    USMountainTime = 2,
    USArizona = 3,
    USPacific = 4
};

struct config_data
{
    config_data()
    {
        setDefaults();
    }

    void setDefaults()
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);

        const auto defaultUserIDPassword = "admin";
        host_name_.clear();
        web_user_name_ = defaultUserIDPassword;
        web_password_ = defaultUserIDPassword;
        wifi_ssid_.clear();
        wifi_password_.clear();
        ntp_server_.clear();
        ntp_server_refresh_interval_ = 60 * 60 * 1000;
        time_zone_ = TimeZoneSupported::USPacific;
        manual_screen_brightness_.reset();
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

    std::string get_web_user_name() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return web_user_name_;
    }
    void set_web_user_name(const std::string &web_user_name)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        web_user_name_ = web_user_name;
    }

    std::string get_web_password() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return web_password_;
    }
    void set_web_password(const std::string &web_password)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        web_password_ = web_password;
    }

    std::string get_wifi_ssid() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return wifi_ssid_;
    }
    void set_wifi_ssid(const std::string &wifi_ssid)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        wifi_ssid_ = wifi_ssid;
    }

    std::string get_wifi_password() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return wifi_password_;
    }
    void set_wifi_password(const std::string &wifi_password)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        wifi_password_ = wifi_password;
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

    std::string get_ntp_server() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return ntp_server_;
    }
    void set_ntp_server(const std::string &ntp_server)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        ntp_server_ = ntp_server;
    }

    uint64_t get_ntp_server_refresh_interval() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return ntp_server_refresh_interval_;
    }
    void set_ntp_server_refresh_interval(uint64_t ntp_server_refresh_interval)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        ntp_server_refresh_interval_ = ntp_server_refresh_interval;
    }

    TimeZoneSupported get_timezone() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        return time_zone_;
    }
    void set_timezone(TimeZoneSupported time_zone)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        time_zone_ = time_zone;
    }

private:
    std::string host_name_;
    std::string web_user_name_;
    std::string web_password_;
    std::string wifi_ssid_;
    std::string wifi_password_;

    std::string ntp_server_;
    uint64_t ntp_server_refresh_interval_;
    TimeZoneSupported time_zone_;

    std::optional<uint8_t> manual_screen_brightness_;

    mutable esp32::semaphore data_mutex_;
};

class config : public esp32::change_callback
{
public:
    bool pre_begin();
    void save();
    void reset();
    void loop();

    static void erase();
    static config instance;

    std::string get_all_config_as_json();

    // does not restore to memory, needs reboot
    bool restore_all_config_as_json(const std::vector<uint8_t> &json,
                                    const std::string &md5);

    config_data data;

private:
    config() = default;
    static std::string read_file(const char *file_name);
    size_t write_to_file(const char *fileName,
                         const void *data,
                         size_t _size);

    template <class T, class JDoc>
    bool deserialize_to_json(const T &data, JDoc &jsonDocument);

    static std::string md5_hash(const std::string &data);

    void save_config();
    std::atomic_bool request_save_{false};
};
