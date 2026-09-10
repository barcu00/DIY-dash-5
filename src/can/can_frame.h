#pragma once

#include <cstdint>

struct CanFrame {
    uint32_t id = 0U;
    uint8_t dlc = 0U;
    uint8_t data[8]{};
    bool extended = false;
    bool remote = false;
};
