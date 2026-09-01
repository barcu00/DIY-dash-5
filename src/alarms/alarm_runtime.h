#pragma once

#include "alarms/alarm_manager.h"
#include "settings/app_config.h"
#include "telemetry/parameter_registry.h"

class AlarmRuntime {
public:
    void update(const ParameterRegistry& registry, const AppConfig& config,
                AlarmManager& manager, uint32_t now_ms);

    AlarmState highestState() const { return highest_state_; }
    ParameterId highestParameter() const { return highest_parameter_; }

private:
    AlarmState highest_state_ = AlarmState::Normal;
    ParameterId highest_parameter_ = ParameterId::Rpm;
};
