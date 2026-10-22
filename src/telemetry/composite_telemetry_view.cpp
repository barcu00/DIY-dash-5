#include "composite_telemetry_view.h"

#include "odometer/odometer_model.h"
#include "racechrono/racechrono_channel_catalog.h"

CompositeTelemetryView::CompositeTelemetryView(
    const VehicleState& engine, const RaceChronoTelemetry& racechrono,
    const OdometerModel* odometer)
    : engine_(engine), racechrono_(racechrono), odometer_(odometer) {}

const SignalValue& CompositeTelemetryView::get(ParameterId id) const {
    if (isOdometerParameter(id) && odometer_ != nullptr) {
        return odometer_->get(id);
    }
    return isRaceChronoParameter(id) ? racechrono_.get(id) : engine_.get(id);
}

bool CompositeTelemetryView::availableFromSupplement(ParameterId id) const {
    if (isOdometerParameter(id)) {
        return odometer_ != nullptr && odometer_->get(id).valid;
    }
    return isRaceChronoParameter(id) && racechrono_.get(id).valid;
}

const VehicleState& CompositeTelemetryView::engineState() const {
    return engine_;
}

const RaceChronoTelemetry& CompositeTelemetryView::raceChronoState() const {
    return racechrono_;
}
