#pragma once

#include <cstdint>

#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"

enum class TemperatureBarZone : uint8_t {
    Unavailable,
    Cold,
    Normal,
    Warm,
    Hot,
};

struct TemperatureBarState {
    bool visible = false;
    uint16_t fill_per_mille = 0U;
    TemperatureBarZone zone = TemperatureBarZone::Unavailable;
};

TemperatureBarState temperatureBarState(
    ParameterId parameter, const SignalValue& signal,
    const TemperatureBarConfig& config);
