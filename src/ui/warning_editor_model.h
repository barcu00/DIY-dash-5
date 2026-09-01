#pragma once

#include <cstdint>

#include "alarms/warning_config.h"

class WarningEditorModel {
public:
    static void setMode(WarningConfig& config, WarningMode mode);
    static void setHighThresholds(WarningConfig& config, float warning, float critical);
    static void setLowThresholds(WarningConfig& config, float warning, float critical);
    static void setRangeThresholds(WarningConfig& config,
                                   float warning_a, float warning_b,
                                   float critical_a, float critical_b);
    static void setHysteresis(WarningConfig& config, float hysteresis);
    static void setDelayMs(WarningConfig& config, uint32_t delay_ms);

    static bool addRpmPoint(WarningConfig& config, float rpm, float warning, float critical);
    static bool updateRpmPoint(WarningConfig& config, uint8_t index,
                               float rpm, float warning, float critical);
    static bool removeRpmPoint(WarningConfig& config, uint8_t index);
    static void clearRpmCurve(WarningConfig& config);

private:
    static void sortRpmCurve(WarningConfig& config);
};
