#include "temperature_bar_model.h"

#include <algorithm>

#include "telemetry/parameter_registry.h"

TemperatureBarState temperatureBarState(
    ParameterId parameter, const SignalValue& signal,
    const TemperatureBarConfig& config) {
    TemperatureBarState state;
    state.visible = config.enabled &&
        parameterDescriptor(parameter).native_unit == NativeUnit::Celsius;
    if (!state.visible || !signal.valid) {
        return state;
    }

    const float span = config.maximum_native - config.minimum_native;
    if (span <= 0.0f) {
        return state;
    }
    const float normalized = std::clamp(
        (signal.value - config.minimum_native) / span, 0.0f, 1.0f);
    state.fill_per_mille = static_cast<uint16_t>(normalized * 1000.0f + 0.5f);

    if (signal.value < config.ready_native) {
        state.zone = TemperatureBarZone::Cold;
    } else if (signal.value >= config.red_native) {
        state.zone = TemperatureBarZone::Hot;
    } else {
        const float warm_start = config.red_native -
            (config.red_native - config.ready_native) * 0.25f;
        state.zone = signal.value >= warm_start
            ? TemperatureBarZone::Warm
            : TemperatureBarZone::Normal;
    }
    return state;
}
