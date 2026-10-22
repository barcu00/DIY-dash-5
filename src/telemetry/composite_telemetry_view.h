#pragma once

#include "racechrono/racechrono_telemetry.h"
#include "telemetry/vehicle_state.h"

class OdometerModel;

class CompositeTelemetryView {
public:
    CompositeTelemetryView(const VehicleState& engine,
                           const RaceChronoTelemetry& racechrono,
                           const OdometerModel* odometer = nullptr);

    const SignalValue& get(ParameterId id) const;
    bool availableFromSupplement(ParameterId id) const;
    const VehicleState& engineState() const;
    const RaceChronoTelemetry& raceChronoState() const;

private:
    const VehicleState& engine_;
    const RaceChronoTelemetry& racechrono_;
    const OdometerModel* odometer_;
};
