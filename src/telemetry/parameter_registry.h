#pragma once

#include <cstdint>

#include "telemetry/parameter_id.h"

enum class ParameterKind : uint8_t {
    Numeric,
    Flag,
};

struct ParameterDescriptor {
    ParameterId id;
    const char* name;
    const char* short_name;
    NativeUnit native_unit;
    uint8_t default_decimals;
    ParameterKind kind = ParameterKind::Numeric;
};

const ParameterDescriptor& parameterDescriptor(ParameterId id);

// Native-unit bounds for a tile warning. Replaces the former global
// 0..999 limit, which excluded RPM, EGT and negative (vacuum, timing)
// thresholds.
struct WarningLimits {
    float minimum;
    float maximum;
    float hysteresis_maximum;
};
WarningLimits warningLimits(ParameterId id);
