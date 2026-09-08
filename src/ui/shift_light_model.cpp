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
        constexpr uint32_t kFlashPeriodMs = 125U;
        constexpr uint32_t kRedPhaseMs = 63U;
        const bool red_phase = (now_ms % kFlashPeriodMs) < kRedPhaseMs;
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

    for (std::size_t i = 0U; i < lit_count; ++i) {
        states[i].lit = true;
        states[i].color = i < 4U   ? ShiftColor::Green
                          : i < 8U ? ShiftColor::Yellow
                                   : ShiftColor::Red;
    }
    return states;
}
