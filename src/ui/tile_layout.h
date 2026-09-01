#pragma once

#include <cstdint>

struct TilePlacement {
    int16_t x = 10;
    int16_t y = 42;
    int16_t width = 202;
    int16_t height = 58;
};

class TileLayout {
public:
    static constexpr uint8_t kSlotCount = 12U;

    static TilePlacement placement(uint8_t slot) {
        if (slot >= kSlotCount) slot = 0U;
        const bool right = slot >= 6U;
        const uint8_t row = static_cast<uint8_t>(slot % 6U);
        return {
            static_cast<int16_t>(right ? 588 : 10),
            static_cast<int16_t>(42 + static_cast<int16_t>(row) * 65),
            202,
            58,
        };
    }
};
