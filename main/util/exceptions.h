#pragma once

#include "util/helper.h"

#include <esp_err.h>
#include <source_location>
#include <stdexcept>
#include <string>

namespace esp32
{
class esp_exception : public std::exception
{
  public:
    esp_exception(const std::source_location &location, const std::string &message, esp_err_t error) : error_(error)
    {
        message_.reserve(256);
        message_.append(location.function_name());
        message_.append(":");
        message_.append(esp32::string::to_string(location.line()));
        message_.append(" Message");
        message_.append(message);
        message_.append(" Error:");
        message_.append(esp_err_to_name(error));
    }

    esp_exception(const std::source_location &location, esp_err_t error) : error_(error)
    {
        message_.reserve(128);
        message_.append(location.function_name());
        message_.append(":");
        message_.append(esp32::string::to_string(location.line()));
        message_.append(" Error:");
        message_.append(esp_err_to_name(error));
    }

    esp_exception(esp_err_t error) : error_(error)
    {
    }

    virtual ~esp_exception() = default;

    const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override
    {
        return message_.empty() ? esp_err_to_name(error_) : message_.c_str();
    }

    esp_err_t get_error() const
    {
        return error_;
    }

  private:
    std::string message_;
    const esp_err_t error_;
};
} // namespace esp32

#define CHECK_THROW(error_, exception_type_)                                                                                                         \
    do                                                                                                                                               \
    {                                                                                                                                                \
        const esp_err_t result = (error_);                                                                                                           \
        if (result != ESP_OK) [[unlikely]]                                                                                                           \
            throw exception_type_(std::source_location::current(), result);                                                                          \
    } while (0)

#define CHECK_THROW2(error_, message, exception_type_)                                                                                               \
    do                                                                                                                                               \
    {                                                                                                                                                \
        const esp_err_t result = (error_);                                                                                                           \
        if (result != ESP_OK) [[unlikely]]                                                                                                           \
            throw exception_type_(std::source_location::current(), message, result);                                                                 \
    } while (0)

#define CHECK_THROW_ESP(error_) CHECK_THROW(error_, esp32::esp_exception);
#define CHECK_THROW_ESP2(error_, message_) CHECK_THROW2(error_, message_, esp32::esp_exception);
