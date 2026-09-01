#include "alarm_manager.h"

#include <algorithm>

namespace {
float nonNegative(float value) {
    return value < 0.0f ? 0.0f : value;
}
}

AlarmState AlarmManager::evaluate(ParameterId id, const ParameterValue& value, float rpm,
                                  const WarningConfig& config, uint32_t now_ms) {
    const std::size_t idx = index(id);
    if (idx >= states_.size()) {
        return AlarmState::Invalid;
    }

    RuntimeState& runtime = states_[idx];

    if (config.mode == WarningMode::Off) {
        runtime = RuntimeState{};
        return AlarmState::Normal;
    }

    if (!value.valid) {
        runtime = RuntimeState{};
        return AlarmState::Invalid;
    }

    const AlarmState desired = desiredState(runtime, value.value, rpm, config);

    if (severity(desired) <= severity(runtime.state)) {
        runtime.state = desired;
        runtime.pending_active = false;
        runtime.pending = AlarmState::Normal;
        runtime.pending_since_ms = 0U;
        return runtime.state;
    }

    if (config.delay_ms == 0U) {
        runtime.state = desired;
        runtime.pending_active = false;
        runtime.pending = AlarmState::Normal;
        runtime.pending_since_ms = 0U;
        return runtime.state;
    }

    if (!runtime.pending_active || runtime.pending != desired) {
        runtime.pending_active = true;
        runtime.pending = desired;
        runtime.pending_since_ms = now_ms;
        return runtime.state;
    }

    if (static_cast<uint32_t>(now_ms - runtime.pending_since_ms) >= config.delay_ms) {
        runtime.state = desired;
        runtime.pending_active = false;
        runtime.pending = AlarmState::Normal;
        runtime.pending_since_ms = 0U;
    }

    return runtime.state;
}

void AlarmManager::reset(ParameterId id) {
    const std::size_t idx = index(id);
    if (idx < states_.size()) {
        states_[idx] = RuntimeState{};
    }
}

void AlarmManager::resetAll() {
    for (auto& state : states_) {
        state = RuntimeState{};
    }
}

AlarmState AlarmManager::state(ParameterId id) const {
    const std::size_t idx = index(id);
    return idx < states_.size() ? states_[idx].state : AlarmState::Invalid;
}

float AlarmManager::rpmWarningThreshold(const WarningConfig& config, float rpm) const {
    return interpolateCurve(config, rpm, false);
}

float AlarmManager::rpmCriticalThreshold(const WarningConfig& config, float rpm) const {
    return interpolateCurve(config, rpm, true);
}

AlarmState AlarmManager::desiredState(const RuntimeState& runtime, float value, float rpm,
                                      const WarningConfig& config) const {
    switch (config.mode) {
        case WarningMode::Off:
            return AlarmState::Normal;
        case WarningMode::High:
            return desiredHigh(runtime, value, config);
        case WarningMode::Low:
            return desiredLow(runtime, value, config.warning_threshold,
                              config.critical_threshold, nonNegative(config.hysteresis));
        case WarningMode::Range:
            return desiredRange(runtime, value, config);
        case WarningMode::RpmCurve:
            return desiredLow(runtime, value,
                              rpmWarningThreshold(config, rpm),
                              rpmCriticalThreshold(config, rpm),
                              nonNegative(config.hysteresis));
    }
    return AlarmState::Normal;
}

AlarmState AlarmManager::desiredHigh(const RuntimeState& runtime, float value,
                                     const WarningConfig& config) const {
    const float hysteresis = nonNegative(config.hysteresis);

    if (runtime.state == AlarmState::Critical) {
        if (value >= config.critical_threshold - hysteresis) {
            return AlarmState::Critical;
        }
        if (value >= config.warning_threshold - hysteresis) {
            return AlarmState::Warning;
        }
        return AlarmState::Normal;
    }

    if (runtime.state == AlarmState::Warning) {
        if (value >= config.critical_threshold) {
            return AlarmState::Critical;
        }
        if (value >= config.warning_threshold - hysteresis) {
            return AlarmState::Warning;
        }
        return AlarmState::Normal;
    }

    if (value >= config.critical_threshold) {
        return AlarmState::Critical;
    }
    if (value >= config.warning_threshold) {
        return AlarmState::Warning;
    }
    return AlarmState::Normal;
}

AlarmState AlarmManager::desiredLow(const RuntimeState& runtime, float value,
                                    float warning_threshold, float critical_threshold,
                                    float hysteresis) const {
    if (runtime.state == AlarmState::Critical) {
        if (value <= critical_threshold + hysteresis) {
            return AlarmState::Critical;
        }
        if (value <= warning_threshold + hysteresis) {
            return AlarmState::Warning;
        }
        return AlarmState::Normal;
    }

    if (runtime.state == AlarmState::Warning) {
        if (value <= critical_threshold) {
            return AlarmState::Critical;
        }
        if (value <= warning_threshold + hysteresis) {
            return AlarmState::Warning;
        }
        return AlarmState::Normal;
    }

    if (value <= critical_threshold) {
        return AlarmState::Critical;
    }
    if (value <= warning_threshold) {
        return AlarmState::Warning;
    }
    return AlarmState::Normal;
}

AlarmState AlarmManager::desiredRange(const RuntimeState& runtime, float value,
                                      const WarningConfig& config) const {
    const float hysteresis = nonNegative(config.hysteresis);

    if (runtime.state == AlarmState::Critical) {
        const bool keep_critical = value <= config.critical_low + hysteresis ||
                                   value >= config.critical_high - hysteresis;
        if (keep_critical) {
            return AlarmState::Critical;
        }
        const bool keep_warning = value <= config.warning_low + hysteresis ||
                                  value >= config.warning_high - hysteresis;
        return keep_warning ? AlarmState::Warning : AlarmState::Normal;
    }

    if (runtime.state == AlarmState::Warning) {
        if (value <= config.critical_low || value >= config.critical_high) {
            return AlarmState::Critical;
        }
        const bool keep_warning = value <= config.warning_low + hysteresis ||
                                  value >= config.warning_high - hysteresis;
        return keep_warning ? AlarmState::Warning : AlarmState::Normal;
    }

    if (value <= config.critical_low || value >= config.critical_high) {
        return AlarmState::Critical;
    }
    if (value <= config.warning_low || value >= config.warning_high) {
        return AlarmState::Warning;
    }
    return AlarmState::Normal;
}

float AlarmManager::interpolateCurve(const WarningConfig& config, float rpm, bool critical) const {
    const uint8_t count = std::min<uint8_t>(config.rpm_curve_count,
                                            WarningConfig::kMaxRpmCurvePoints);
    if (count == 0U) {
        return 0.0f;
    }

    const auto threshold = [critical](const RpmWarningPoint& point) {
        return critical ? point.critical : point.warning;
    };

    if (count == 1U || rpm <= config.rpm_curve[0].rpm) {
        return threshold(config.rpm_curve[0]);
    }

    for (uint8_t i = 1U; i < count; ++i) {
        const RpmWarningPoint& left = config.rpm_curve[i - 1U];
        const RpmWarningPoint& right = config.rpm_curve[i];
        if (rpm <= right.rpm) {
            const float span = right.rpm - left.rpm;
            if (span <= 0.0f) {
                return threshold(right);
            }
            const float t = (rpm - left.rpm) / span;
            return threshold(left) + (threshold(right) - threshold(left)) * t;
        }
    }

    return threshold(config.rpm_curve[count - 1U]);
}

uint8_t AlarmManager::severity(AlarmState state) {
    switch (state) {
        case AlarmState::Normal: return 0U;
        case AlarmState::Warning: return 1U;
        case AlarmState::Critical: return 2U;
        case AlarmState::Invalid: return 0U;
    }
    return 0U;
}

std::size_t AlarmManager::index(ParameterId id) {
    return static_cast<std::size_t>(parameterIndex(id));
}
