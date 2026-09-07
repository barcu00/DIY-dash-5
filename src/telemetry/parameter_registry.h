#pragma once

#include <cstdint>

#include "telemetry/parameter_id.h"

struct ParameterDescriptor {
    ParameterId id;
    const char* name;
    const char* short_name;
    NativeUnit native_unit;
    uint8_t default_decimals;
};

const ParameterDescriptor& parameterDescriptor(ParameterId id);
