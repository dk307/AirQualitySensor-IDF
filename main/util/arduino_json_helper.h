#pragma once

#define ARDUINOJSON_ENABLE_STRING_VIEW 1
#include <ArduinoJson.h>

namespace ArduinoJson
{
template <typename T, typename _Alloc> struct Converter<std::vector<T, _Alloc>>
{
    static void toJson(const std::vector<T, _Alloc> &src, JsonVariant dst)
    {
        JsonArray array = dst.to<JsonArray>();
        for (T item : src)
            array.add(item);
    }
};

template <typename T> struct Converter<std::optional<T>>
{
    static void toJson(const std::optional<T> &src, JsonVariant dst)
    {
        if (src.has_value())
        {
            dst.set(src.value());
        }
        else
        {
            dst.set(nullptr);
        }
    }
};

} // namespace ArduinoJson
