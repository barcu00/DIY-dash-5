#include "gauge_model.h"

#include <algorithm>
#include <cmath>

#include "telemetry/parameter_registry.h"

namespace {
uint16_t fraction(float value, const GaugeRange& range) {
    const float span = range.maximum - range.minimum;
    if (!(span > 0.0f) || !std::isfinite(value)) return 0U;
    const float f = std::clamp((value - range.minimum) / span, 0.0f, 1.0f);
    return static_cast<uint16_t>(std::lround(f * 1000.0f));
}
}  // namespace

GaugeRange gaugeRange(const TileConfig& tile) {
    GaugeRange range;
    const NativeUnit unit = parameterDescriptor(tile.parameter).native_unit;
    switch (unit) {
        case NativeUnit::Celsius:
            range = {tile.temperature_bar.minimum_native,
                     tile.temperature_bar.maximum_native};
            if (!(range.maximum > range.minimum)) range = {40.0f, 140.0f};
            break;
        case NativeUnit::Bar: range = {0.0f, 10.0f}; break;
        case NativeUnit::Rpm: range = {0.0f, 8000.0f}; break;
        case NativeUnit::Kph: range = {0.0f, 300.0f}; break;
        case NativeUnit::Volt: range = {8.0f, 16.0f}; break;
        case NativeUnit::Lambda: range = {0.6f, 1.4f}; break;
        case NativeUnit::Degrees: range = {-10.0f, 50.0f}; break;
        case NativeUnit::GForce: range = {-2.0f, 2.0f}; break;
        default: range = {0.0f, 100.0f}; break;
    }
    if (unit == NativeUnit::Celsius && tile.temperature_bar.red_native > range.minimum)
        range.red_from_permille = fraction(tile.temperature_bar.red_native, range);
    if (tile.warning.enabled) {
        const uint16_t at = fraction(tile.warning.threshold_native, range);
        if (tile.warning.direction == WarningDirection::Above)
            range.red_from_permille = std::min<uint16_t>(range.red_from_permille, at);
        else
            range.red_below_permille = at;
    }
    return range;
}

uint16_t gaugeFill(float native_value, const GaugeRange& range) {
    return fraction(native_value, range);
}
