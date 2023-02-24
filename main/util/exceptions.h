#pragma once

#include <esp_err.h>
#include <stdexcept>
#include <string>

namespace esp32
{
class esp_exception : public std::exception
{
  public:
    esp_exception(const char *extra_message, esp_err_t error) : error_(error)
    {
        message_.append(extra_message);
        message_.append(" Error:");
        message_.append(esp_err_to_name(error));
    }

    esp_exception(const std::string &extra_message, esp_err_t error) : error_(error)
    {
        message_.append(extra_message);
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

class init_failure_exception : public esp_exception
{
  public:
    using esp_exception::esp_exception;
};

class http_request_exception : public esp_exception
{
  public:
    using esp_exception::esp_exception;
};
} // namespace esp32

#define CHECK_THROW(error_, message, exception_type_)                                                                                                \
    do                                                                                                                                               \
    {                                                                                                                                                \
        const esp_err_t result = (error_);                                                                                                           \
        if (result != ESP_OK)                                                                                                                        \
            throw exception_type_(message, result);                                                                                                  \
    } while (0)
