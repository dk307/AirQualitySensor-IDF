#pragma once

#include "util/exceptions.h"
#include "util/noncopyable.h"
#include <nvs.h>
#include <nvs_flash.h>
#include <string>
#include <string_view>
#include <type_traits>

class preferences : esp32::noncopyable
{
  public:
    ~preferences()
    {
        end();
    }

    void begin(const std::string_view &part_name, const std::string_view &namespace_name);
    void end();
    void commit();

    void save(const std::string_view &key, uint8_t value);
    void save(const std::string_view &key, bool value);
    void save(const std::string_view &key, const std::string_view &value);
    void save(const std::string_view &key, const std::string &value);

    bool get(const std::string_view &key, bool default_value);
    uint8_t get(const std::string_view &key, uint8_t default_value);
    std::string get(const std::string_view &key, const std::string_view &default_value);

  private:
    uint32_t handle_{};
    bool started_{false};
};