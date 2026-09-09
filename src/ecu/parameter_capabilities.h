#pragma once

#include <array>
#include <cstddef>

#include "ecu/can_profile.h"
#include "telemetry/parameter_registry.h"
#include "telemetry/vehicle_state.h"

class ParameterCapabilities {
public:
    static ParameterCapabilities forSource(DataSource source,
                                           const CanProfile* profile);

    std::size_t count() const;
    ParameterId at(std::size_t index) const;
    bool supports(ParameterId parameter) const;

private:
    std::array<ParameterId, parameterCount()> parameters_{};
    std::size_t count_ = 0U;
};
