#include "warning_editor_model.h"

#include <algorithm>

void WarningEditorModel::setMode(WarningConfig& config, WarningMode mode) {
    config.mode = mode;
}

void WarningEditorModel::setHighThresholds(WarningConfig& config, float warning, float critical) {
    config.warning_threshold = std::min(warning, critical);
    config.critical_threshold = std::max(warning, critical);
}

void WarningEditorModel::setLowThresholds(WarningConfig& config, float warning, float critical) {
    config.warning_threshold = std::max(warning, critical);
    config.critical_threshold = std::min(warning, critical);
}

void WarningEditorModel::setRangeThresholds(WarningConfig& config,
                                            float warning_a, float warning_b,
                                            float critical_a, float critical_b) {
    config.warning_low = std::min(warning_a, warning_b);
    config.warning_high = std::max(warning_a, warning_b);
    config.critical_low = std::min(critical_a, critical_b);
    config.critical_high = std::max(critical_a, critical_b);
}

void WarningEditorModel::setHysteresis(WarningConfig& config, float hysteresis) {
    config.hysteresis = std::max(0.0f, hysteresis);
}

void WarningEditorModel::setDelayMs(WarningConfig& config, uint32_t delay_ms) {
    config.delay_ms = delay_ms;
}

bool WarningEditorModel::addRpmPoint(WarningConfig& config, float rpm, float warning, float critical) {
    if (config.rpm_curve_count >= WarningConfig::kMaxRpmCurvePoints) {
        return false;
    }

    config.rpm_curve[config.rpm_curve_count++] = {rpm, warning, critical};
    sortRpmCurve(config);
    return true;
}

bool WarningEditorModel::updateRpmPoint(WarningConfig& config, uint8_t index,
                                        float rpm, float warning, float critical) {
    if (index >= config.rpm_curve_count) {
        return false;
    }

    config.rpm_curve[index] = {rpm, warning, critical};
    sortRpmCurve(config);
    return true;
}

bool WarningEditorModel::removeRpmPoint(WarningConfig& config, uint8_t index) {
    if (index >= config.rpm_curve_count) {
        return false;
    }

    for (uint8_t i = index; i + 1U < config.rpm_curve_count; ++i) {
        config.rpm_curve[i] = config.rpm_curve[i + 1U];
    }
    --config.rpm_curve_count;
    config.rpm_curve[config.rpm_curve_count] = {};
    return true;
}

void WarningEditorModel::clearRpmCurve(WarningConfig& config) {
    config.rpm_curve.fill({});
    config.rpm_curve_count = 0U;
}

void WarningEditorModel::sortRpmCurve(WarningConfig& config) {
    std::sort(config.rpm_curve.begin(), config.rpm_curve.begin() + config.rpm_curve_count,
              [](const RpmWarningPoint& a, const RpmWarningPoint& b) {
                  return a.rpm < b.rpm;
              });
}
