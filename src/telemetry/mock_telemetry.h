#pragma once

#include <cstdint>
#include "vehicle_state.h"

class MockTelemetry {
public:
    MockTelemetry();

    void reset();
    void update(uint32_t elapsed_ms);
    const VehicleState& state() const;

private:
    VehicleState state_{};
};
