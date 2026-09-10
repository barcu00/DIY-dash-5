#include "alarm_manager.h"

#include "config/dashboard_config.h"

namespace {
void raise(AlarmSummary& summary, AlarmId alarm, AlarmSeverity severity) {
    const uint32_t mask = static_cast<uint32_t>(alarm);
    summary.active_mask |= mask;
    if (severity == AlarmSeverity::Critical) {
        summary.critical_mask |= mask;
    } else if (severity == AlarmSeverity::Warning) {
        summary.warning_mask |= mask;
    }
    if (static_cast<uint8_t>(severity) > static_cast<uint8_t>(summary.severity)) {
        summary.severity = severity;
    }
}

bool valid(const VehicleState& state, VehicleSignal signal) {
    return state.get(signal).valid;
}

float value(const VehicleState& state, VehicleSignal signal) {
    return state.get(signal).value;
}
}

AlarmSummary AlarmManager::evaluate(const VehicleState& state) const {
    AlarmSummary summary;

    if (valid(state, VehicleSignal::Clt)) {
        const float clt = value(state, VehicleSignal::Clt);
        if (clt >= DashboardConfig::kCltCriticalC) {
            raise(summary, AlarmId::HighClt, AlarmSeverity::Critical);
        } else if (clt >= DashboardConfig::kCltWarningC) {
            raise(summary, AlarmId::HighClt, AlarmSeverity::Warning);
        }
    }

    if (valid(state, VehicleSignal::Iat)) {
        const float iat = value(state, VehicleSignal::Iat);
        if (iat >= DashboardConfig::kIatCriticalC) {
            raise(summary, AlarmId::HighIat, AlarmSeverity::Critical);
        } else if (iat >= DashboardConfig::kIatWarningC) {
            raise(summary, AlarmId::HighIat, AlarmSeverity::Warning);
        }
    }

    if (valid(state, VehicleSignal::OilPressure)) {
        const float oil = value(state, VehicleSignal::OilPressure);
        if (oil <= DashboardConfig::kOilPressureCriticalBar) {
            raise(summary, AlarmId::LowOilPressure, AlarmSeverity::Critical);
        } else if (oil <= DashboardConfig::kOilPressureWarningBar) {
            raise(summary, AlarmId::LowOilPressure, AlarmSeverity::Warning);
        }
    }

    if (valid(state, VehicleSignal::Lambda) && valid(state, VehicleSignal::Map) &&
        valid(state, VehicleSignal::Tps) &&
        value(state, VehicleSignal::Map) >= DashboardConfig::kLeanLoadMapBar &&
        value(state, VehicleSignal::Tps) >= DashboardConfig::kLeanLoadTpsPercent) {
        const float lambda = value(state, VehicleSignal::Lambda);
        if (lambda >= DashboardConfig::kLeanLambdaCritical) {
            raise(summary, AlarmId::LeanLambda, AlarmSeverity::Critical);
        } else if (lambda >= DashboardConfig::kLeanLambdaWarning) {
            raise(summary, AlarmId::LeanLambda, AlarmSeverity::Warning);
        }
    }

    if (valid(state, VehicleSignal::BatteryVoltage)) {
        const float battery = value(state, VehicleSignal::BatteryVoltage);
        if (battery <= DashboardConfig::kBatteryCriticalV) {
            raise(summary, AlarmId::LowBattery, AlarmSeverity::Critical);
        } else if (battery <= DashboardConfig::kBatteryWarningV) {
            raise(summary, AlarmId::LowBattery, AlarmSeverity::Warning);
        }
    }

    return summary;
}
