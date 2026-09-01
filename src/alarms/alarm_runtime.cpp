#include "alarm_runtime.h"

namespace {
uint8_t severity(AlarmState state) {
    switch (state) {
        case AlarmState::Warning: return 1U;
        case AlarmState::Critical: return 2U;
        case AlarmState::Normal:
        default: return 0U;
    }
}
}

void AlarmRuntime::update(const ParameterRegistry& registry, const AppConfig& config,
                          AlarmManager& manager, uint32_t now_ms) {
    highest_state_ = AlarmState::Normal;
    highest_parameter_ = ParameterId::Rpm;

    const ParameterValue& rpm_value = registry.value(ParameterId::Rpm);
    const float rpm = rpm_value.valid ? rpm_value.value : 0.0f;

    for (uint16_t i = 0; i < AppConfig::kParameterCount; ++i) {
        const ParameterId id = static_cast<ParameterId>(i);
        const AlarmState state = manager.evaluate(id, registry.value(id), rpm,
                                                  config.warnings[i], now_ms);
        if (severity(state) > severity(highest_state_)) {
            highest_state_ = state;
            highest_parameter_ = id;
        }
    }
}
