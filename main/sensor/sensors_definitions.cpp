#include "sensor.h"

#include <limits>

const std::array<sensor_definition_display, 1> no_level{
    sensor_definition_display{-999, 999, 0},
};

const std::array<sensor_definition_display, 6> pm_2_5_definition_display{
    sensor_definition_display{std::numeric_limits<uint32_t>::min(), 12, 0},
    sensor_definition_display{12, 35.4, 1},
    sensor_definition_display{35.4, 55.4, 2},
    sensor_definition_display{55.4, 150.4, 3},
    sensor_definition_display{150.4, 250.4, 4},
    sensor_definition_display{250.4, std::numeric_limits<uint32_t>::max(), 5},
};

const std::array<sensor_definition_display, 1> pm_1_definition_display{
    sensor_definition_display{-999, 999, 0},
};

const std::array<sensor_definition_display, 1> pm_4_definition_display{
    sensor_definition_display{-999, 999, 0},
};

const std::array<sensor_definition_display, 1> voc_definition_display{
    sensor_definition_display{-99, 99, 0},
};

const std::array<sensor_definition_display, 6> pm_10_definition_display{
    sensor_definition_display{std::numeric_limits<uint32_t>::min(), 54, 0},
    sensor_definition_display{55, 154, 1},
    sensor_definition_display{154, 254, 2},
    sensor_definition_display{254, 354, 3},
    sensor_definition_display{354, 424, 4},
    sensor_definition_display{424, std::numeric_limits<uint32_t>::max(), 5},
};

const std::array<sensor_definition_display, 3> co2_definition_display{
    sensor_definition_display{0, 1000, 0},
    sensor_definition_display{1000, 1500, 1},
    sensor_definition_display{1500, 2000, 2},
};

const std::array<sensor_definition_display, 5> temperature_definition_display{
    sensor_definition_display{-99, 32, 2},
    sensor_definition_display{32, 60, 1},
    sensor_definition_display{60, 80, 0},
    sensor_definition_display{80, 100, 1},
    sensor_definition_display{100, 999, 2},
};

const std::array<sensor_definition_display, 4> humidity_definition_display{
    sensor_definition_display{0, 20, 1},
    sensor_definition_display{20, 60, 0},
    sensor_definition_display{60, 80, 1},
    sensor_definition_display{80, 100, 2},
};

const std::array<sensor_definition, total_sensors> sensor_definitions{
    sensor_definition{"PM 2.5", "µg/m³", pm_2_5_definition_display.data(), pm_2_5_definition_display.size()},
    // sensor_definition{"VOC", "ppb", voc_definition_display.data(), voc_definition_display.size()},
    sensor_definition{"Temperature", "°F", temperature_definition_display.data(), temperature_definition_display.size()},
    sensor_definition{"Humidity", "⁒", humidity_definition_display.data(), humidity_definition_display.size()},
    // sensor_definition{"eCO2", "ppm", co2_definition_display.data(), co2_definition_display.size()},
    sensor_definition{"PM 1", "µg/m³", pm_1_definition_display.data(), pm_1_definition_display.size()},
    sensor_definition{"PM 4", "µg/m³", pm_4_definition_display.data(), pm_4_definition_display.size()},
    sensor_definition{"PM 10", "µg/m³", pm_10_definition_display.data(), pm_10_definition_display.size()},
    sensor_definition{"Typical Particle Size", "µg", no_level.data(), no_level.size()},
    sensor_definition{"Light Intensity", "lux", no_level.data(), no_level.size()},
};

const sensor_definition &get_sensor_definition(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)];
}
const char *get_sensor_name(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)].get_name();
}

const char *get_sensor_unit(sensor_id_index id)
{
    return sensor_definitions[static_cast<size_t>(id)].get_unit();
}