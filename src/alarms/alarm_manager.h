#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "alarms/warning_config.h"
#include "telemetry/parameter_id.h"
#include "telemetry/parameter_registry.h"

class AlarmManager {
public:
    AlarmState evaluate(ParameterId id, const ParameterValue& value, float rpm,
                        const WarningConfig& config, uint32_t now_ms);

    void reset(ParameterId id);
    void resetAll();

    AlarmState state(ParameterId id) const;

    float rpmWarningThreshold(const WarningConfig& config, float rpm) const;
    float rpmCriticalThreshold(const WarningConfig& config, float rpm) const;

private:
    struct RuntimeState {
        AlarmState state = AlarmState::Normal;
        AlarmState pending = AlarmState::Normal;
        uint32_t pending_since_ms = 0U;
        bool pending_active = false;
    };

    static constexpr std::size_t kParameterCount = static_cast<std::size_t>(ParameterId::Count);

    AlarmState desiredState(const RuntimeState& runtime, float value, float rpm,
                            const WarningConfig& config) const;
    AlarmState desiredHigh(const RuntimeState& runtime, float value,
                           const WarningConfig& config) const;
    AlarmState desiredLow(const RuntimeState& runtime, float value,
                          float warning_threshold, float critical_threshold,
                          float hysteresis) const;
    AlarmState desiredRange(const RuntimeState& runtime, float value,
                            const WarningConfig& config) const;

    float interpolateCurve(const WarningConfig& config, float rpm, bool critical) const;
    static uint8_t severity(AlarmState state);
    static std::size_t index(ParameterId id);

    std::array<RuntimeState, kParameterCount> states_{};
};
