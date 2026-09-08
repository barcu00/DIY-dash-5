#include "display_signal_filter.h"

#include <algorithm>

void DisplaySignalFilter::reset() {
    initialized_ = false;
    start_value_ = 0.0f;
    target_value_ = 0.0f;
    transition_start_ms_ = 0U;
    transition_ms_ = 1U;
    raw_updated_ms_ = 0U;
}

float DisplaySignalFilter::valueAt(uint32_t now_ms) const {
    const uint32_t elapsed = now_ms - transition_start_ms_;
    if (elapsed >= transition_ms_) {
        return target_value_;
    }
    const float progress = static_cast<float>(elapsed) /
                           static_cast<float>(transition_ms_);
    return start_value_ + (target_value_ - start_value_) * progress;
}

SignalValue DisplaySignalFilter::sample(const SignalValue& raw,
                                        uint32_t now_ms,
                                        uint32_t transition_ms) {
    if (!raw.valid) {
        reset();
        return SignalValue{};
    }

    if (!initialized_) {
        initialized_ = true;
        start_value_ = raw.value;
        target_value_ = raw.value;
        transition_start_ms_ = now_ms;
        transition_ms_ = 1U;
        raw_updated_ms_ = raw.updated_ms;
        return raw;
    }

    if (raw.updated_ms != raw_updated_ms_) {
        start_value_ = valueAt(now_ms);
        target_value_ = raw.value;
        transition_start_ms_ = now_ms;
        transition_ms_ = std::max<uint32_t>(transition_ms, 1U);
        raw_updated_ms_ = raw.updated_ms;
    }

    return SignalValue{valueAt(now_ms), raw.updated_ms, true};
}
