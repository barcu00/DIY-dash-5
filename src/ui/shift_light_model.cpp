#include "shift_light_model.h"

#include <algorithm>
#include <cstddef>

ShiftSegmentStates ShiftLightModel::segments(
    uint16_t rpm, bool rpm_valid, uint32_t now_ms,
    const ShiftLightConfig& config) {
    ShiftSegmentStates states{};
    if (!rpm_valid || !(config.start_rpm < config.red_rpm &&
                        config.red_rpm < config.flash_rpm &&
                        config.flash_rpm <= config.max_rpm)) {
        return states;
    }

    constexpr std::size_t kSegmentCount = 12U;
    if (config.flash_enabled && rpm >= config.flash_rpm) {
        const bool red_phase = ((now_ms / 125U) % 2U) == 0U;
        if (red_phase) {
            for (ShiftSegmentState& state : states) {
                state.lit = true;
                state.color = ShiftColor::Red;
            }
        }
        return states;
    }

    const uint32_t range =
        static_cast<uint32_t>(config.max_rpm - config.start_rpm);
    std::size_t lit_count = 0U;
    if (rpm >= config.max_rpm) {
        lit_count = kSegmentCount;
    } else if (rpm >= config.start_rpm) {
        const uint32_t progress =
            static_cast<uint32_t>(rpm - config.start_rpm);
        lit_count = 1U + static_cast<std::size_t>(
            (progress * kSegmentCount) / range);
        lit_count = std::min(lit_count, kSegmentCount);
    }

    std::size_t red_index = static_cast<std::size_t>(
        (static_cast<uint32_t>(config.red_rpm - config.start_rpm) *
         kSegmentCount) /
        range);
    red_index = std::min(red_index, kSegmentCount - 1U);
    const std::size_t yellow_index = red_index > 1U ? red_index - 2U : 0U;

    for (std::size_t i = 0U; i < lit_count; ++i) {
        states[i].lit = true;
        if (i >= red_index) {
            states[i].color = ShiftColor::Red;
        } else if (i >= yellow_index) {
            states[i].color = ShiftColor::Yellow;
        } else {
            states[i].color = ShiftColor::Green;
        }
    }
    return states;
}
