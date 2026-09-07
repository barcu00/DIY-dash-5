#pragma once

#include <array>
#include <cstdint>

#include "settings/app_config.h"

enum class ShiftColor : uint8_t {
    Off,
    Green,
    Yellow,
    Red,
};

struct ShiftSegmentState {
    bool lit = false;
    ShiftColor color = ShiftColor::Off;
};

using ShiftSegmentStates = std::array<ShiftSegmentState, 12>;

class ShiftLightModel {
public:
    static ShiftSegmentStates segments(uint16_t rpm,
                                       const ShiftLightConfig& config);
};
