#pragma once

#include "settings/app_config.h"

class ParameterPickerModel {
public:
    static ParameterId nextVisible(const AppConfig& config, ParameterId current);
    static ParameterId previousVisible(const AppConfig& config, ParameterId current);
    static bool setVisible(AppConfig& config, ParameterId parameter, bool visible);

private:
    static uint16_t visibleCount(const AppConfig& config);
};
