#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <cstring>
#include <type_traits>
#include <vector>
#include <optional>

//copied from https://github.com/esphome/esphome/tree/dev/esphome/core

namespace esp32
{
    /// Compare strings for equality in case-insensitive manner.
    bool str_equals_case_insensitive(const std::string &a, const std::string &b);

    /// Check whether a string starts with a value.
    bool str_startswith(const std::string &str, const std::string &start);
    /// Check whether a string ends with a value.
    bool str_endswith(const std::string &str, const std::string &end);

    /// Convert the value to a string (added as extra overload so that to_string() can be used on all stringifiable types).
    inline std::string to_string(const std::string &val) { return val; }

    /// Truncate a string to a specific length.
    std::string str_truncate(const std::string &str, size_t length);

    /// Extract the part of the string until either the first occurrence of the specified character, or the end
    /// (requires str to be null-terminated).
    std::string str_until(const char *str, char ch);
    /// Extract the part of the string until either the first occurrence of the specified character, or the end.
    std::string str_until(const std::string &str, char ch);

    /// Convert the string to lower case.
    std::string str_lower_case(const std::string &str);
    /// Convert the string to upper case.
    std::string str_upper_case(const std::string &str);
    /// Convert the string to snake case (lowercase with underscores).
    std::string str_snake_case(const std::string &str);

    /// Sanitizes the input string by removing all characters but alphanumerics, dashes and underscores.
    std::string str_sanitize(const std::string &str);

    /// snprintf-like function returning std::string of maximum length \p len (excluding null terminator).
    std::string __attribute__((format(printf, 1, 3))) str_snprintf(const char *fmt, size_t len, ...);

    /// sprintf-like function returning std::string.
    std::string __attribute__((format(printf, 1, 2))) str_sprintf(const char *fmt, ...);

    ///@}

    /// @name Parsing & formatting
    ///@{

    /// Parse an unsigned decimal number from a null-terminated string.
    template <typename T, std::enable_if_t<(std::is_integral<T>::value && std::is_unsigned<T>::value), int> = 0>
    std::optional<T> parse_number(const char *str)
    {
        char *end = nullptr;
        unsigned long value = ::strtoul(str, &end, 10); // NOLINT(google-runtime-int)
        if (end == str || *end != '\0' || value > std::numeric_limits<T>::max())
            return {};
        return value;
    }
    /// Parse an unsigned decimal number.
    template <typename T, std::enable_if_t<(std::is_integral<T>::value && std::is_unsigned<T>::value), int> = 0>
    std::optional<T> parse_number(const std::string &str)
    {
        return parse_number<T>(str.c_str());
    }
    /// Parse a signed decimal number from a null-terminated string.
    template <typename T, std::enable_if_t<(std::is_integral<T>::value && std::is_signed<T>::value), int> = 0>
    std::optional<T> parse_number(const char *str)
    {
        char *end = nullptr;
        signed long value = ::strtol(str, &end, 10); // NOLINT(google-runtime-int)
        if (end == str || *end != '\0' || value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max())
            return {};
        return value;
    }
    /// Parse a signed decimal number.
    template <typename T, std::enable_if_t<(std::is_integral<T>::value && std::is_signed<T>::value), int> = 0>
    std::optional<T> parse_number(const std::string &str)
    {
        return parse_number<T>(str.c_str());
    }
    /// Parse a decimal floating-point number from a null-terminated string.
    template <typename T, std::enable_if_t<(std::is_same<T, float>::value), int> = 0>
    std::optional<T> parse_number(const char *str)
    {
        char *end = nullptr;
        float value = ::strtof(str, &end);
        if (end == str || *end != '\0' || value == HUGE_VALF)
            return {};
        return value;
    }
    /// Parse a decimal floating-point number.
    template <typename T, std::enable_if_t<(std::is_same<T, float>::value), int> = 0>
    std::optional<T> parse_number(const std::string &str)
    {
        return parse_number<T>(str.c_str());
    }

    /** Parse bytes from a hex-encoded string into a byte array.
     *
     * When \p len is less than \p 2*count, the result is written to the back of \p data (i.e. this function treats \p str
     * as if it were padded with zeros at the front).
     *
     * @param str String to read from.
     * @param len Length of \p str (excluding optional null-terminator), is a limit on the number of characters parsed.
     * @param data Byte array to write to.
     * @param count Length of \p data.
     * @return The number of characters parsed from \p str.
     */
    size_t parse_hex(const char *str, size_t len, uint8_t *data, size_t count);
    /// Parse \p count bytes from the hex-encoded string \p str of at least \p 2*count characters into array \p data.
    inline bool parse_hex(const char *str, uint8_t *data, size_t count)
    {
        return parse_hex(str, std::strlen(str), data, count) == 2 * count;
    }
    /// Parse \p count bytes from the hex-encoded string \p str of at least \p 2*count characters into array \p data.
    inline bool parse_hex(const std::string &str, uint8_t *data, size_t count)
    {
        return parse_hex(str.c_str(), str.length(), data, count) == 2 * count;
    }
    /// Parse \p count bytes from the hex-encoded string \p str of at least \p 2*count characters into vector \p data.
    inline bool parse_hex(const char *str, std::vector<uint8_t> &data, size_t count)
    {
        data.resize(count);
        return parse_hex(str, std::strlen(str), data.data(), count) == 2 * count;
    }
    /// Parse \p count bytes from the hex-encoded string \p str of at least \p 2*count characters into vector \p data.
    inline bool parse_hex(const std::string &str, std::vector<uint8_t> &data, size_t count)
    {
        data.resize(count);
        return parse_hex(str.c_str(), str.length(), data.data(), count) == 2 * count;
    }
    /** Parse a hex-encoded string into an unsigned integer.
     *
     * @param str String to read from, starting with the most significant byte.
     * @param len Length of \p str (excluding optional null-terminator), is a limit on the number of characters parsed.
     */
    template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
    std::optional<T> parse_hex(const char *str, size_t len)
    {
        T val = 0;
        if (len > 2 * sizeof(T) || parse_hex(str, len, reinterpret_cast<uint8_t *>(&val), sizeof(T)) == 0)
            return {};
        return convert_big_endian(val);
    }
    /// Parse a hex-encoded null-terminated string (starting with the most significant byte) into an unsigned integer.
    template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
    std::optional<T> parse_hex(const char *str)
    {
        return parse_hex<T>(str, strlen(str));
    }
    /// Parse a hex-encoded null-terminated string (starting with the most significant byte) into an unsigned integer.
    template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
    std::optional<T> parse_hex(const std::string &str)
    {
        return parse_hex<T>(str.c_str(), str.length());
    }

    /// Format the byte array \p data of length \p len in lowercased hex.
    std::string format_hex(const uint8_t *data, size_t length);
    /// Format the vector \p data in lowercased hex.
    std::string format_hex(const std::vector<uint8_t> &data);
    /// Format an unsigned integer in lowercased hex, starting with the most significant byte.
    template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
    std::string format_hex(T val)
    {
        val = convert_big_endian(val);
        return format_hex(reinterpret_cast<uint8_t *>(&val), sizeof(T));
    }

    /// Format the byte array \p data of length \p len in pretty-printed, human-readable hex.
    std::string format_hex_pretty(const uint8_t *data, size_t length);
    /// Format the word array \p data of length \p len in pretty-printed, human-readable hex.
    std::string format_hex_pretty(const uint16_t *data, size_t length);
    /// Format the vector \p data in pretty-printed, human-readable hex.
    std::string format_hex_pretty(const std::vector<uint8_t> &data);
    /// Format the vector \p data in pretty-printed, human-readable hex.
    std::string format_hex_pretty(const std::vector<uint16_t> &data);
    /// Format an unsigned integer in pretty-printed, human-readable hex, starting with the most significant byte.
    template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0>
    std::string format_hex_pretty(T val)
    {
        val = convert_big_endian(val);
        return format_hex_pretty(reinterpret_cast<uint8_t *>(&val), sizeof(T));
    }

    /// Return values for parse_on_off().
    enum ParseOnOffState
    {
        PARSE_NONE = 0,
        PARSE_ON,
        PARSE_OFF,
        PARSE_TOGGLE,
    };
    /// Parse a string that contains either on, off or toggle.
    ParseOnOffState parse_on_off(const char *str, const char *on = nullptr, const char *off = nullptr);

    /// Create a string from a value and an accuracy in decimals.
    std::string value_accuracy_to_string(float value, int8_t accuracy_decimals);

    /// Derive accuracy in decimals from an increment step.
    int8_t step_to_accuracy_decimals(float step);

}
