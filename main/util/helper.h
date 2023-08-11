#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

// copied from https://github.com/esphome/esphome/tree/dev/esphome/core

namespace esp32
{

namespace string
{

/// snprintf-like function returning std::string of maximum length \p len
/// (excluding null terminator).
std::string __attribute__((format(printf, 1, 3))) snprintf(const char *fmt, size_t len, ...);

/// sprintf-like function returning std::string.
std::string __attribute__((format(printf, 1, 2))) sprintf(const char *fmt, ...);

inline std::string to_string(int value)
{
    return snprintf("%d", 32, value);
}
inline std::string to_string(long value)
{
    return snprintf("%ld", 32, value);
}
inline std::string to_string(long long value)
{
    return snprintf("%lld", 32, value);
}
inline std::string to_string(unsigned value)
{
    return snprintf("%u", 32, value);
}
inline std::string to_string(unsigned long value)
{
    return snprintf("%lu", 32, value);
}
inline std::string to_string(unsigned long long value)
{
    return snprintf("%llu", 32, value);
}
inline std::string to_string(float value)
{
    return snprintf("%g", 32, value);
}
inline std::string to_string(double value)
{
    return snprintf("%g", 32, value);
}
inline std::string to_string(long double value)
{
    return snprintf("%Lf", 32, value);
}

std::string stringify_size(uint64_t bytes, int max_unit = 128);

/// Compare strings for equality in case-insensitive manner.
bool equals_case_insensitive(const std::string &a, const std::string &b);

///@}

/// @name Parsing & formatting
///@{

/// Parse an unsigned decimal number from a null-terminated string.
template <typename T>
std::optional<T> parse_number(const std::string_view &str)
    requires(std::is_integral_v<T> && std::is_unsigned_v<T>)
{
    char *end = nullptr;
    const auto value = ::strtoull(str.data(), &end, 10);
    if (end == str || *end != '\0' || value > std::numeric_limits<T>::max())
        return {};
    return value;
}

/// Parse a signed decimal number from a null-terminated string.
template <typename T>
std::optional<T> parse_number(const std::string_view &str)
    requires(std::is_integral_v<T> && std::is_signed_v<T>)
{
    char *end = nullptr;
    const auto value = ::strtoll(str.data(), &end, 10);
    if (end == str || *end != '\0' || value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max())
        return {};
    return value;
}

/// Parse a decimal floating-point number from a null-terminated string.
template <typename T>
std::optional<T> parse_number(const std::string_view &str)
    requires(std::is_integral_v<T> && std::is_same_v<T, float>)
{
    char *end = nullptr;
    const auto value = ::strtof(str.data(), &end);
    if (end == str || *end != '\0' || value == HUGE_VALF)
        return {};
    return value;
}

} // namespace string

// std::byteswap from C++23
template <typename T> constexpr T byteswap(T n)
{
    T m;
    for (size_t i = 0; i < sizeof(T); i++)
        reinterpret_cast<uint8_t *>(&m)[i] = reinterpret_cast<uint8_t *>(&n)[sizeof(T) - 1 - i];
    return m;
}
template <> constexpr uint8_t byteswap(uint8_t n)
{
    return n;
}
template <> constexpr uint16_t byteswap(uint16_t n)
{
    return __builtin_bswap16(n);
}
template <> constexpr uint32_t byteswap(uint32_t n)
{
    return __builtin_bswap32(n);
}
template <> constexpr uint64_t byteswap(uint64_t n)
{
    return __builtin_bswap64(n);
}
template <> constexpr int8_t byteswap(int8_t n)
{
    return n;
}
template <> constexpr int16_t byteswap(int16_t n)
{
    return __builtin_bswap16(n);
}
template <> constexpr int32_t byteswap(int32_t n)
{
    return __builtin_bswap32(n);
}
template <> constexpr int64_t byteswap(int64_t n)
{
    return __builtin_bswap64(n);
}

/// Convert a value between host byte order and big endian (most significant
/// byte first) order.
template <typename T> constexpr T convert_big_endian(T val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return byteswap(val);
#else
    return val;
#endif
}

/// Convert a value between host byte order and little endian (least significant
/// byte first) order.
template <typename T> constexpr T convert_little_endian(T val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return val;
#else
    return byteswap(val);
#endif
}

inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s)
{
    rtrim(s);
    ltrim(s);
}

// trim from start (copying)
inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

/// Check whether a string starts with a value.
bool str_startswith(const std::string &str, const std::string &start);
/// Check whether a string ends with a value.
bool str_endswith(const std::string &str, const std::string &end);

/// Convert the value to a string (added as extra overload so that to_string()
/// can be used on all stringifiable types).
inline std::string to_string(const std::string &val)
{
    return val;
}

/// Truncate a string to a specific length.
std::string str_truncate(const std::string &str, size_t length);

/// Extract the part of the string until either the first occurrence of the
/// specified character, or the end (requires str to be null-terminated).
std::string str_until(const char *str, char ch);
/// Extract the part of the string until either the first occurrence of the
/// specified character, or the end.
std::string str_until(const std::string &str, char ch);

/// Convert the string to lower case.
std::string str_lower_case(const std::string &str);
/// Convert the string to upper case.
std::string str_upper_case(const std::string &str);
/// Convert the string to snake case (lowercase with underscores).
std::string str_snake_case(const std::string &str);

/// Sanitizes the input string by removing all characters but alphanumerics,
/// dashes and underscores.
std::string str_sanitize(const std::string &str);

/** Parse bytes from a hex-encoded string into a byte array.
 *
 * When \p len is less than \p 2*count, the result is written to the back of \p
 * data (i.e. this function treats \p str as if it were padded with zeros at the
 * front).
 *
 * @param str String to read from.
 * @param len Length of \p str (excluding optional null-terminator), is a limit
 * on the number of characters parsed.
 * @param data Byte array to write to.
 * @param count Length of \p data.
 * @return The number of characters parsed from \p str.
 */
size_t parse_hex(const char *str, size_t len, uint8_t *data, size_t count);

/// Parse \p count bytes from the hex-encoded string \p str of at least \p
/// 2*count characters into array \p data.
inline bool parse_hex(const char *str, uint8_t *data, size_t count)
{
    return parse_hex(str, std::strlen(str), data, count) == 2 * count;
}

/// Parse \p count bytes from the hex-encoded string \p str of at least \p
/// 2*count characters into array \p data.
inline bool parse_hex(const std::string &str, uint8_t *data, size_t count)
{
    return parse_hex(str.c_str(), str.length(), data, count) == 2 * count;
}

/// Parse \p count bytes from the hex-encoded string \p str of at least \p
/// 2*count characters into vector \p data.
inline bool parse_hex(const char *str, std::vector<uint8_t> &data, size_t count)
{
    data.resize(count);
    return parse_hex(str, std::strlen(str), data.data(), count) == 2 * count;
}

/// Parse \p count bytes from the hex-encoded string \p str of at least \p
/// 2*count characters into vector \p data.
inline bool parse_hex(const std::string &str, std::vector<uint8_t> &data, size_t count)
{
    data.resize(count);
    return parse_hex(str.c_str(), str.length(), data.data(), count) == 2 * count;
}

/** Parse a hex-encoded string into an unsigned integer.
 *
 * @param str String to read from, starting with the most significant byte.
 * @param len Length of \p str (excluding optional null-terminator), is a limit
 * on the number of characters parsed.
 */
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0> std::optional<T> parse_hex(const char *str, size_t len)
{
    T val = 0;
    if (len > 2 * sizeof(T) || parse_hex(str, len, reinterpret_cast<uint8_t *>(&val), sizeof(T)) == 0)
        return {};
    return esp32::convert_big_endian(val);
}

/// Parse a hex-encoded null-terminated string (starting with the most
/// significant byte) into an unsigned integer.
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0> std::optional<T> parse_hex(const char *str)
{
    return parse_hex<T>(str, strlen(str));
}

/// Parse a hex-encoded null-terminated string (starting with the most
/// significant byte) into an unsigned integer.
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0> std::optional<T> parse_hex(const std::string &str)
{
    return parse_hex<T>(str.c_str(), str.length());
}

/// Format the byte array \p data of length \p len in lowercased hex.
std::string format_hex(const uint8_t *data, size_t length);
/// Format the vector \p data in lowercased hex.
std::string format_hex(const std::vector<uint8_t> &data);
/// Format an unsigned integer in lowercased hex, starting with the most
/// significant byte.
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0> std::string format_hex(T val)
{
    return format_hex(reinterpret_cast<uint8_t *>(&val), sizeof(T));
}

/// Format the byte array \p data of length \p len in pretty-printed,
/// human-readable hex.
std::string format_hex_pretty(const uint8_t *data, size_t length);
/// Format the word array \p data of length \p len in pretty-printed,
/// human-readable hex.
std::string format_hex_pretty(const uint16_t *data, size_t length);
/// Format the vector \p data in pretty-printed, human-readable hex.
std::string format_hex_pretty(const std::vector<uint8_t> &data);
/// Format the vector \p data in pretty-printed, human-readable hex.
std::string format_hex_pretty(const std::vector<uint16_t> &data);
/// Format an unsigned integer in pretty-printed, human-readable hex, starting
/// with the most significant byte.
template <typename T, std::enable_if_t<std::is_unsigned<T>::value, int> = 0> std::string format_hex_pretty(T val)
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

} // namespace esp32
