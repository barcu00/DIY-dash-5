#pragma once

#include <cstdint>

#include "telemetry/vehicle_state.h"

enum class AlarmId : uint32_t {
    HighClt = 1U << 0U,
    HighIat = 1U << 1U,
    LowOilPressure = 1U << 2U,
    LeanLambda = 1U << 3U,
    LowBattery = 1U << 4U,
};

enum class AlarmSeverity : uint8_t {
    None,
    Warning,
    Critical,
};

struct AlarmSummary {
    uint32_t active_mask = 0U;
    uint32_t warning_mask = 0U;
    uint32_t critical_mask = 0U;
    AlarmSeverity severity = AlarmSeverity::None;

    bool active(AlarmId alarm) const {
        return (active_mask & static_cast<uint32_t>(alarm)) != 0U;
    }

    AlarmSeverity severityFor(AlarmId alarm) const {
        const uint32_t mask = static_cast<uint32_t>(alarm);
        if ((critical_mask & mask) != 0U) {
            return AlarmSeverity::Critical;
        }
        if ((warning_mask & mask) != 0U) {
            return AlarmSeverity::Warning;
        }
        return AlarmSeverity::None;
    }
};

class AlarmManager {
public:
    AlarmSummary evaluate(const VehicleState& state) const;
};
