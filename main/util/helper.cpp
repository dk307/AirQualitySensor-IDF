#include "helper.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>

namespace esp32
{

namespace string
{
std::string snprintf(const char *fmt, size_t len, ...)
{
    std::string str;
    va_list args;

    str.resize(len);
    va_start(args, len);
    size_t out_length = vsnprintf(&str[0], len + 1, fmt, args);
    va_end(args);

    if (out_length < len)
        str.resize(out_length);

    return str;
}

std::string sprintf(const char *fmt, ...)
{
    std::string str;
    va_list args;

    va_start(args, fmt);
    size_t length = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    str.resize(length);
    va_start(args, fmt);
    vsnprintf(&str[0], length + 1, fmt, args);
    va_end(args);

    return str;
}

std::string stringify_size(uint64_t bytes, int max_unit)
{
    constexpr char suffix[3][3] = {"B", "KB", "MB"};
    constexpr char length = sizeof(suffix) / sizeof(suffix[0]);

    uint16_t i = 0;
    double dblBytes = bytes;

    if (bytes > 1024)
    {
        for (i = 0; (bytes / 1024) > 0 && i < length - 1 && (max_unit > 0); i++, bytes /= 1024)
        {
            dblBytes = bytes / 1024.0;
            max_unit--;
        }
    }

    return sprintf("%llu %s", static_cast<uint64_t>(std::round(dblBytes)), suffix[i]);
}

bool equals_case_insensitive(const std::string &a, const std::string &b)
{
    return strcasecmp(a.c_str(), b.c_str()) == 0;
}

} // namespace string

bool str_startswith(const std::string &str, const std::string &start)
{
    return str.rfind(start, 0) == 0;
}

bool str_endswith(const std::string &str, const std::string &end)
{
    return str.rfind(end) == (str.size() - end.size());
}

std::string str_truncate(const std::string &str, size_t length)
{
    return str.length() > length ? str.substr(0, length) : str;
}

std::string str_until(const char *str, char ch)
{
    char *pos = strchr(str, ch);
    return pos == nullptr ? std::string(str) : std::string(str, pos - str);
}

std::string str_until(const std::string &str, char ch)
{
    return str.substr(0, str.find(ch));
}

// wrapper around std::transform to run safely on functions from the ctype.h
// header see https://en.cppreference.com/w/cpp/string/byte/toupper#Notes
template <int (*fn)(int)> std::string str_ctype_transform(const std::string &str)
{
    std::string result;
    result.resize(str.length());
    std::transform(str.begin(), str.end(), result.begin(), [](unsigned char ch) { return fn(ch); });
    return result;
}
std::string str_lower_case(const std::string &str)
{
    return str_ctype_transform<std::tolower>(str);
}
std::string str_upper_case(const std::string &str)
{
    return str_ctype_transform<std::toupper>(str);
}
std::string str_snake_case(const std::string &str)
{
    std::string result;
    result.resize(str.length());
    std::transform(str.begin(), str.end(), result.begin(), ::tolower);
    std::replace(result.begin(), result.end(), ' ', '_');
    return result;
}

std::string str_sanitize(const std::string &str)
{
    std::string out;
    std::copy_if(str.begin(), str.end(), std::back_inserter(out),
                 [](const char &c) { return c == '-' || c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); });
    return out;
}

// Parsing & formatting

size_t parse_hex(const char *str, size_t length, uint8_t *data, size_t count)
{
    uint8_t val;
    size_t chars = std::min(length, 2 * count);
    for (size_t i = 2 * count - chars; i < 2 * count; i++, str++)
    {
        if (*str >= '0' && *str <= '9')
        {
            val = *str - '0';
        }
        else if (*str >= 'A' && *str <= 'F')
        {
            val = 10 + (*str - 'A');
        }
        else if (*str >= 'a' && *str <= 'f')
        {
            val = 10 + (*str - 'a');
        }
        else
        {
            return 0;
        }
        data[i >> 1] = !(i & 1) ? val << 4 : data[i >> 1] | val;
    }
    return chars;
}

static char format_hex_char(uint8_t v)
{
    return v >= 10 ? 'a' + (v - 10) : '0' + v;
}
std::string format_hex(const uint8_t *data, size_t length)
{
    std::string ret;
    ret.resize(length * 2);
    for (size_t i = 0; i < length; i++)
    {
        ret[2 * i] = format_hex_char((data[i] & 0xF0) >> 4);
        ret[2 * i + 1] = format_hex_char(data[i] & 0x0F);
    }
    return ret;
}
std::string format_hex(const std::vector<uint8_t> &data)
{
    return format_hex(data.data(), data.size());
}

static char format_hex_pretty_char(uint8_t v)
{
    return v >= 10 ? 'A' + (v - 10) : '0' + v;
}
std::string format_hex_pretty(const uint8_t *data, size_t length)
{
    if (length == 0)
        return "";
    std::string ret;
    ret.resize(3 * length - 1);
    for (size_t i = 0; i < length; i++)
    {
        ret[3 * i] = format_hex_pretty_char((data[i] & 0xF0) >> 4);
        ret[3 * i + 1] = format_hex_pretty_char(data[i] & 0x0F);
        if (i != length - 1)
            ret[3 * i + 2] = '.';
    }
    if (length > 4)
        return ret + " (" + esp32::string::to_string(length) + ")";
    return ret;
}
std::string format_hex_pretty(const std::vector<uint8_t> &data)
{
    return format_hex_pretty(data.data(), data.size());
}

std::string format_hex_pretty(const uint16_t *data, size_t length)
{
    if (length == 0)
        return "";
    std::string ret;
    ret.resize(5 * length - 1);
    for (size_t i = 0; i < length; i++)
    {
        ret[5 * i] = format_hex_pretty_char((data[i] & 0xF000) >> 12);
        ret[5 * i + 1] = format_hex_pretty_char((data[i] & 0x0F00) >> 8);
        ret[5 * i + 2] = format_hex_pretty_char((data[i] & 0x00F0) >> 4);
        ret[5 * i + 3] = format_hex_pretty_char(data[i] & 0x000F);
        if (i != length - 1)
            ret[5 * i + 2] = '.';
    }
    if (length > 4)
        return ret + " (" + esp32::string::to_string(length) + ")";
    return ret;
}
std::string format_hex_pretty(const std::vector<uint16_t> &data)
{
    return format_hex_pretty(data.data(), data.size());
}

ParseOnOffState parse_on_off(const char *str, const char *on, const char *off)
{
    if (on == nullptr && strcasecmp(str, "on") == 0)
        return PARSE_ON;
    if (on != nullptr && strcasecmp(str, on) == 0)
        return PARSE_ON;
    if (off == nullptr && strcasecmp(str, "off") == 0)
        return PARSE_OFF;
    if (off != nullptr && strcasecmp(str, off) == 0)
        return PARSE_OFF;
    if (strcasecmp(str, "toggle") == 0)
        return PARSE_TOGGLE;

    return PARSE_NONE;
}

std::string value_accuracy_to_string(float value, int8_t accuracy_decimals)
{
    if (accuracy_decimals < 0)
    {
        auto multiplier = powf(10.0f, accuracy_decimals);
        value = roundf(value * multiplier) / multiplier;
        accuracy_decimals = 0;
    }
    char tmp[32]; // should be enough, but we should maybe improve this at some
                  // point.
    snprintf(tmp, sizeof(tmp), "%.*f", accuracy_decimals, value);
    return std::string(tmp);
}

int8_t step_to_accuracy_decimals(float step)
{
    // use printf %g to find number of digits based on temperature step
    char buf[32];
    sprintf(buf, "%.5g", step);

    std::string str{buf};
    size_t dot_pos = str.find('.');
    if (dot_pos == std::string::npos)
        return 0;

    return str.length() - dot_pos - 1;
}

} // namespace esp32
