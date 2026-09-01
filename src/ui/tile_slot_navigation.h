#pragma once

#include <cstdint>

class TileSlotNavigation {
public:
    static constexpr uint8_t kSlotCount = 12U;

    static uint8_t normalize(uint8_t slot) {
        return slot < kSlotCount ? slot : 0U;
    }

    static uint8_t next(uint8_t slot) {
        return static_cast<uint8_t>((normalize(slot) + 1U) % kSlotCount);
    }

    static uint8_t previous(uint8_t slot) {
        const uint8_t normalized = normalize(slot);
        return normalized == 0U ? static_cast<uint8_t>(kSlotCount - 1U)
                                : static_cast<uint8_t>(normalized - 1U);
    }
};
