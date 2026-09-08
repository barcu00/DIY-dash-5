#pragma once

#include <cstdint>

#include "telemetry/vehicle_state.h"

class DisplaySignalFilter {
public:
    SignalValue sample(const SignalValue& raw, uint32_t now_ms,
                       uint32_t transition_ms);
    void reset();

private:
    float valueAt(uint32_t now_ms) const;

    bool initialized_ = false;
    float start_value_ = 0.0f;
    float target_value_ = 0.0f;
    uint32_t transition_start_ms_ = 0U;
    uint32_t transition_ms_ = 1U;
    uint32_t raw_updated_ms_ = 0U;
};
