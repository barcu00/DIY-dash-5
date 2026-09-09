#pragma once

#include <cstddef>
#include <array>

#include "ecu/parameter_capabilities.h"
#include "telemetry/parameter_registry.h"

class ParameterOptionList {
public:
    bool write(char* output, std::size_t capacity) const;
    std::size_t count() const;
    ParameterId parameterAt(std::size_t index) const;

private:
    friend class ParameterOptions;
    std::array<ParameterId, parameterCount() + 1U> parameters_{};
    std::size_t count_ = 0U;
    bool first_unavailable_ = false;
};

class ParameterOptions {
public:
    static bool write(char* output, std::size_t capacity);
    static ParameterOptionList build(DataSource source,
                                     const CanProfile* profile,
                                     ParameterId current);
};
