#pragma once

#include <cstddef>

#include "telemetry/parameter_registry.h"

class ParameterOptions {
public:
    static bool write(char* output, std::size_t capacity);
};
