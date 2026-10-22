#include "distance_source.h"

#include "telemetry/composite_telemetry_view.h"

namespace {
SpeedSample fromSignal(const SignalValue& signal) {
    return SpeedSample{signal.value, signal.valid};
}

SpeedSample wheelAverage(const CompositeTelemetryView& state) {
    constexpr ParameterId kWheels[] = {
        ParameterId::WheelSpeedLf, ParameterId::WheelSpeedRf,
        ParameterId::WheelSpeedLr, ParameterId::WheelSpeedRr};
    float sum = 0.0f;
    unsigned count = 0U;
    for (const ParameterId wheel : kWheels) {
        const SignalValue& signal = state.get(wheel);
        if (signal.valid) {
            sum += signal.value;
            ++count;
        }
    }
    return count == 0U ? SpeedSample{} : SpeedSample{sum / count, true};
}
}  // namespace

SpeedSample selectDistanceSpeed(const OdometerSettings& settings,
                                const CompositeTelemetryView& state,
                                bool engine_data_simulated) {
    const SpeedSample gps = fromSignal(state.get(ParameterId::RcGpsSpeed));
    if (settings.source == DistanceSource::Gps) return gps;

    SpeedSample primary{};
    if (!engine_data_simulated) {
        primary = settings.source == DistanceSource::Wheels
                      ? wheelAverage(state)
                      : fromSignal(state.get(ParameterId::Speed));
    }
    if (primary.valid) return primary;
    return settings.gps_fallback ? gps : SpeedSample{};
}
