#pragma once

#include <stdexcept>
#include <string>
namespace esp32
{

    class esp_exception : public std::exception
    {
    public:
        esp_exception(const char *extra_message, esp_err_t error) : error(error)
        {
            message.append(extra_message);
            message.append(" Error:");
            message.append(esp_err_to_name(error));
        }
        esp_exception(esp_err_t error) : error(error) {}
        virtual ~esp_exception() = default;
        const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override
        {
            return message.empty() ? esp_err_to_name(error) : message.c_str();
        }

        esp_err_t get_error() const
        {
            return error;
        }

    private:
        std::string message;
        const esp_err_t error;
    };

    class init_failure_exception : public esp_exception
    {
    public:
        using esp_exception::esp_exception;
    };

}

#define CHECK_THROW(message, error_, exception_type_)      \
    do                                                     \
    {                                                      \
        const esp_err_t result = (error_);                 \
        if (result != ESP_OK)                              \
            throw esp32::exception_type_(message, result); \
    } while (0)
