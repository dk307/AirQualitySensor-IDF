#pragma once

#include "hardware/sensors/sensor_id.h"
#include "util/circular_buffer.h"
#include "util/psram_allocator.h"
#include "util/semaphore_lockable.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <optional>
#include <type_traits>
#include <vector>

class sensor_definition_display
{
  public:
    constexpr sensor_definition_display(double range_min, double range_max, sensor_level level) noexcept
        : range_min{range_min}, range_max{range_max}, level{level}
    {
    }

    constexpr double get_range_min() const noexcept
    {
        return range_min;
    }

    constexpr double get_range_max() const noexcept
    {
        return range_max;
    }

    constexpr sensor_level get_level() const noexcept
    {
        return level;
    }

    constexpr bool is_in_range(double value) const noexcept
    {
        return range_min <= value && value < range_max;
    }

  public:
    const double range_min;
    const double range_max;
    const sensor_level level;
};

class sensor_definition
{
  public:
    constexpr sensor_definition(const std::string_view &name, const std::string_view &unit, const sensor_definition_display *display_definitions,
                                size_t display_definitions_count, float min_value, float max_value, float value_step) noexcept
        : name_{name}, unit_(unit), display_definitions_(display_definitions), display_definitions_count_(display_definitions_count),
          min_value_(min_value), max_value_(max_value), value_step_(value_step)
    {
    }

    constexpr sensor_level calculate_level(float value_p) const noexcept
    {
        for (uint8_t i = 0; i < display_definitions_count_; i++)
        {
            if (display_definitions_[i].is_in_range(value_p))
            {
                return display_definitions_[i].get_level() + 1;
            }
        }
        return 0;
    }

    constexpr auto &&get_unit() const noexcept
    {
        return unit_;
    }
    constexpr auto &&get_name() const noexcept
    {
        return name_;
    }

    constexpr float get_max_value() const noexcept
    {
        return max_value_;
    }

    constexpr float get_min_value() const noexcept
    {
        return min_value_;
    }

    constexpr float get_value_step() const noexcept
    {
        return value_step_;
    }

  private:
    const std::string_view name_;
    const std::string_view unit_;
    const sensor_definition_display *display_definitions_;
    const uint8_t display_definitions_count_;
    const float min_value_;
    const float max_value_;
    const float value_step_;
};

class sensor_value
{
  public:
    std::optional<float> get_value() const
    {
        const auto value = value_.load();
        if (std::isnan(value))
        {
            return std::nullopt;
        }
        return value;
    }

    template <class T>
    std::optional<T> get_value_as() const
        requires std::is_integral_v<T>
    {
        const auto value = value_.load();
        if (std::isnan(value))
        {
            return std::nullopt;
        }
        return std::lround<T>(value);
    }

    bool set_value(float value)
    {
        return set_value_(value);
    }

    bool set_invalid_value()
    {
        return set_value_(NAN);
    }

  private:
    std::atomic<float> value_{NAN};

    /**
     * Returns true if changed
     */
    bool set_value_(float value)
    {
        if (value_.exchange(value) != value)
        {
            return true;
        }
        return false;
    }
};

template <uint16_t countT> class sensor_history_t
{
  public:
    typedef struct
    {
        int16_t mean;
        int16_t min;
        int16_t max;
    } stats;

    using vector_history_t = std::vector<int16_t, esp32::psram::allocator<int16_t>>;

    typedef struct
    {
        std::optional<stats> stat;
        vector_history_t history;
    } sensor_history_snapshot;

    void add_value(float value)
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        last_x_values_.push(value);
    }

    void clear()
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        last_x_values_.clear();
    }

    sensor_history_snapshot get_snapshot(uint8_t group_by_count) const
    {
        vector_history_t return_values;

        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        const auto size = last_x_values_.size();
        if (size)
        {
            return_values.reserve(1 + (size / group_by_count));
            float value_max = std::numeric_limits<float>::min();
            float value_min = std::numeric_limits<float>::max();
            double sum = 0;
            double group_sum = 0;
            for (auto i = 0; i < size; i++)
            {
                const auto value = last_x_values_[i];
                sum += value;
                group_sum += value;
                value_max = std::max<float>(value, value_max);
                value_min = std::min<float>(value, value_min);

                if (((i + 1) % group_by_count) == 0)
                {
                    return_values.push_back(std::lround<int16_t>(group_sum / (group_by_count)));
                    group_sum = 0;
                }
            }

            // add partial group average
            if (size % group_by_count)
            {
                return_values.push_back(std::lround<int16_t>(group_sum / (size % group_by_count)));
            }

            stats stats_value;
            stats_value.max = std::lround<int16_t>(value_max);
            stats_value.min = std::lround<int16_t>(value_min);
            stats_value.mean = std::lround<int16_t>(sum / size);
            return {stats_value, return_values};
        }
        else
        {
            return {std::nullopt, return_values};
        };
    }

    std::optional<int16_t> get_average() const
    {
        std::lock_guard<esp32::semaphore> lock(data_mutex_);
        const auto size = last_x_values_.size();
        if (size)
        {
            double sum = 0;
            for (auto i = 0; i < size; i++)
            {
                sum += last_x_values_[i];
            }
            return static_cast<int16_t>(sum / size);
        }
        else
        {
            return std::nullopt;
        }
    }

  private:
    mutable esp32::semaphore data_mutex_;
    circular_buffer<float, countT> last_x_values_;
};

template <uint8_t reads_per_minuteT, uint16_t minutesT> class sensor_history_minute_t : public sensor_history_t<reads_per_minuteT * minutesT>
{
  public:
    static constexpr auto total_minutes = minutesT;
    static constexpr auto reads_per_minute = reads_per_minuteT;
    static constexpr auto sensor_interval = (60 * 1000 / reads_per_minute);
};

using sensor_history = sensor_history_minute_t<12, 240>;

constexpr std::array<sensor_definition_display, 0> no_level{};

constexpr std::array<sensor_definition_display, 6> pm_2_5_definition_display{
    sensor_definition_display{std::numeric_limits<uint32_t>::min(), 12, 0},
    sensor_definition_display{12, 35.4, 1},
    sensor_definition_display{35.4, 55.4, 2},
    sensor_definition_display{55.4, 150.4, 3},
    sensor_definition_display{150.4, 250.4, 4},
    sensor_definition_display{250.4, std::numeric_limits<uint32_t>::max(), 5},
};

constexpr std::array<sensor_definition_display, 6> pm_10_definition_display{
    sensor_definition_display{std::numeric_limits<uint32_t>::min(), 54, 0},
    sensor_definition_display{55, 154, 1},
    sensor_definition_display{154, 254, 2},
    sensor_definition_display{254, 354, 3},
    sensor_definition_display{354, 424, 4},
    sensor_definition_display{424, std::numeric_limits<uint32_t>::max(), 5},
};

constexpr std::array<sensor_definition, total_sensors> sensor_definitions{
    sensor_definition{"PM 2.5", "µg/m³", pm_2_5_definition_display.data(), pm_2_5_definition_display.size(), 0, 1000, 1},
    sensor_definition{"Temperature", "°F", no_level.data(), no_level.size(), -40, 140, 1},
    sensor_definition{"Temperature", "°C", no_level.data(), no_level.size(), -40, 70, 0.1},
    sensor_definition{"Humidity", "⁒", no_level.data(), no_level.size(), 0, 100, 1},
    sensor_definition{"PM 1", "µg/m³", no_level.data(), no_level.size(), 0, 1000, 1},
    sensor_definition{"PM 4", "µg/m³", no_level.data(), no_level.size(), 0, 1000, 1},
    sensor_definition{"PM 10", "µg/m³", pm_10_definition_display.data(), pm_10_definition_display.size(), 0, 1000, 1},
    sensor_definition{"Typical Particle Size", "µg", no_level.data(), no_level.size(), 0, 10, 0.1},
    sensor_definition{"Light Intensity", "lux", no_level.data(), no_level.size(), 0, 65535, 1},
};

constexpr auto &&get_sensor_definition(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)];
}

constexpr auto &&get_sensor_name(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)].get_name();
}

constexpr auto &&get_sensor_unit(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)].get_unit();
}