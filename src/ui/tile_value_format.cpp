#include "tile_value_format.h"

#include <cmath>
#include <cstdio>

#include "telemetry/parameter_registry.h"

namespace {
bool isDelta(ParameterId id) {
    return id == ParameterId::RcDeltaLapTime || id == ParameterId::RcLapTimeGain;
}
}  // namespace

void formatTileValue(ParameterId id, float presented, uint8_t decimals,
                     char* output, std::size_t capacity) {
    if (output == nullptr || capacity == 0U) return;
    const unsigned places = decimals > 3U ? 3U : decimals;
    if (!std::isfinite(presented)) {
        std::snprintf(output, capacity, "---");
        return;
    }
    if (isDelta(id)) {
        std::snprintf(output, capacity, "%+.*f", places == 0U ? 2U : places,
                      static_cast<double>(presented));
        return;
    }
    if (parameterDescriptor(id).native_unit == NativeUnit::Seconds &&
        std::fabs(presented) >= 60.0f) {
        const double scale = std::pow(10.0, places);
        const long long total = std::llround(std::fabs(presented) * scale);
        const long long whole_seconds = total / static_cast<long long>(scale);
        const long long fraction = total % static_cast<long long>(scale);
        const long long hours = whole_seconds / 3600;
        const long long minutes = (whole_seconds / 60) % 60;
        const long long seconds = whole_seconds % 60;
        const char* sign = presented < 0.0f ? "-" : "";
        if (hours > 0) {
            std::snprintf(output, capacity, "%s%lld:%02lld:%02lld", sign, hours,
                          minutes, seconds);
        } else if (places == 0U) {
            std::snprintf(output, capacity, "%s%lld:%02lld", sign, minutes, seconds);
        } else {
            std::snprintf(output, capacity, "%s%lld:%02lld.%0*lld", sign, minutes,
                          seconds, static_cast<int>(places), fraction);
        }
        return;
    }
    std::snprintf(output, capacity, "%.*f", places, static_cast<double>(presented));
}

DeltaTone deltaTone(ParameterId id, float presented) {
    if (!isDelta(id) || !std::isfinite(presented) || presented == 0.0f)
        return DeltaTone::Neutral;
    return presented < 0.0f ? DeltaTone::Better : DeltaTone::Worse;
}
