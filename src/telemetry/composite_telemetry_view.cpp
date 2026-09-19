#include "composite_telemetry_view.h"

#include "racechrono/racechrono_channel_catalog.h"

CompositeTelemetryView::CompositeTelemetryView(
    const VehicleState& engine, const RaceChronoTelemetry& racechrono)
    : engine_(engine), racechrono_(racechrono) {}

const SignalValue& CompositeTelemetryView::get(ParameterId id) const {
    return isRaceChronoParameter(id) ? racechrono_.get(id) : engine_.get(id);
}

bool CompositeTelemetryView::availableFromSupplement(ParameterId id) const {
    return isRaceChronoParameter(id) && racechrono_.get(id).valid;
}

const VehicleState& CompositeTelemetryView::engineState() const {
    return engine_;
}

const RaceChronoTelemetry& CompositeTelemetryView::raceChronoState() const {
    return racechrono_;
}
