#pragma once

#include <cstdint>

enum class DataSource : uint8_t {
    Mock = 0,
    Ecumaster = 1,
    Rusefi = 2,
};
