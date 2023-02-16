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
        std::lock_guard<esp32::semaphore> lock(data_mutex);

        const auto defaultUserIDPassword = "admin";
        host_name.clear();
        web_user_name = defaultUserIDPassword;
        web_password = defaultUserIDPassword;
        wifi_ssid.clear();
        wifi_password.clear();
        ntp_server.clear();
        ntp_server_refresh_interval = 60 * 60 * 1000;
        time_zone = TimeZoneSupported::USPacific;
        manual_screen_brightness.reset();
    }

    std::string get_host_name() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return host_name;
    }
    void set_host_name(const std::string &host_name_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        host_name = host_name_;
    }

    std::string get_web_user_name() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return web_user_name;
    }
    void set_web_user_name(const std::string &web_user_name_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        web_user_name = web_user_name_;
    }

    std::string get_web_password() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return web_password;
    }
    void set_web_password(const std::string &web_password_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        web_password = web_password_;
    }

    std::string get_wifi_ssid() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return wifi_ssid;
    }
    void set_wifi_ssid(const std::string &wifi_ssid_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        wifi_ssid = wifi_ssid_;
    }

    std::string get_wifi_password() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return wifi_password;
    }
    void set_wifi_password(const std::string &wifi_password_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        wifi_password = wifi_password_;
    }

    std::optional<uint8_t> get_manual_screen_brightness() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return manual_screen_brightness;
    }
    void set_manual_screen_brightness(const std::optional<uint8_t> &screen_brightness_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        manual_screen_brightness = screen_brightness_;
    }

    std::string get_ntp_server() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return ntp_server;
    }
    void set_ntp_server(const std::string &ntp_server_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        ntp_server = ntp_server_;
    }

    uint64_t get_ntp_server_refresh_interval() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return ntp_server_refresh_interval;
    }
    void set_ntp_server_refresh_interval(uint64_t ntp_server_refresh_interval_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        ntp_server_refresh_interval = ntp_server_refresh_interval_;
    }

    TimeZoneSupported get_timezone() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        return time_zone;
    }
    void set_timezone(TimeZoneSupported time_zone_)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex);
        time_zone = time_zone_;
    }

private:
    std::string host_name;
    std::string web_user_name;
    std::string web_password;
    std::string wifi_ssid;
    std::string wifi_password;

    std::string ntp_server;
    uint64_t ntp_server_refresh_interval;
    TimeZoneSupported time_zone;

    std::optional<uint8_t> manual_screen_brightness;

    mutable esp32::semaphore data_mutex;
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
    std::atomic_bool request_save{false};
};
