#include "settings_overlay.h"

#include <cstdio>

#include "ui/warning_editor_model.h"

namespace {
const lv_color_t kWarnText = lv_color_hex(0xFFCF33);
const lv_color_t kCriticalText = lv_color_hex(0xFF3B30);
const lv_color_t kInfoText = lv_color_hex(0x7D8996);

float nonNegative(float value) {
    return value < 0.0f ? 0.0f : value;
}
}

const char* SettingsOverlay::warningModeName(WarningMode mode) {
    switch (mode) {
        case WarningMode::Off: return "OFF";
        case WarningMode::Low: return "LOW";
        case WarningMode::High: return "HIGH";
        case WarningMode::Range: return "RANGE";
        case WarningMode::RpmCurve: return "RPM CURVE";
    }
    return "OFF";
}

WarningConfig* SettingsOverlay::selectedWarning() {
    if (config_ == nullptr || selected_warning_parameter_ >= AppConfig::kParameterCount) return nullptr;
    return &config_->warnings[selected_warning_parameter_];
}

float SettingsOverlay::warningStep() const {
    if (registry_ == nullptr || selected_warning_parameter_ >= AppConfig::kParameterCount) return 0.1f;
    const auto& descriptor = registry_->descriptor(static_cast<ParameterId>(selected_warning_parameter_));
    if (descriptor.decimals == 0U) return 1.0f;
    if (descriptor.decimals == 1U) return 0.1f;
    return 0.01f;
}

void SettingsOverlay::openWarning(ParameterId parameter) {
    const uint16_t index = parameterIndex(parameter);
    selected_warning_parameter_ = index < AppConfig::kParameterCount ? index : 0U;
    selected_curve_point_ = 0U;
    openWarnings();
}

void SettingsOverlay::openWarnings() {
    if (config_ == nullptr || registry_ == nullptr) return;
    if (selected_warning_parameter_ >= AppConfig::kParameterCount) selected_warning_parameter_ = 0U;

    createOverlay("WARNINGS");
    const ParameterId id = static_cast<ParameterId>(selected_warning_parameter_);
    const auto& descriptor = registry_->descriptor(id);
    WarningConfig& warning = config_->warnings[selected_warning_parameter_];

    char line[128];
    std::snprintf(line, sizeof(line), "%s  [%u/%u]", descriptor.name,
                  static_cast<unsigned>(selected_warning_parameter_ + 1U),
                  static_cast<unsigned>(AppConfig::kParameterCount));
    text(line, 24, 52, &lv_font_montserrat_18, kWarnText);
    std::snprintf(line, sizeof(line), "MODE: %s", warningModeName(warning.mode));
    text(line, 24, 80, &lv_font_montserrat_18, kCriticalText);

    if (warning.mode == WarningMode::High || warning.mode == WarningMode::Low) {
        std::snprintf(line, sizeof(line), "WARN %.2f   CRIT %.2f   HYST %.2f   DELAY %u ms",
                      static_cast<double>(warning.warning_threshold),
                      static_cast<double>(warning.critical_threshold),
                      static_cast<double>(warning.hysteresis),
                      static_cast<unsigned>(warning.delay_ms));
        text(line, 24, 110, &lv_font_montserrat_14);
    } else if (warning.mode == WarningMode::Range) {
        std::snprintf(line, sizeof(line), "WARN %.2f..%.2f   CRIT %.2f..%.2f",
                      static_cast<double>(warning.warning_low),
                      static_cast<double>(warning.warning_high),
                      static_cast<double>(warning.critical_low),
                      static_cast<double>(warning.critical_high));
        text(line, 24, 110, &lv_font_montserrat_14);
        text("WARN/CRIT +/- widens or narrows the corresponding band.", 24, 134,
             &lv_font_montserrat_12, kInfoText);
    } else if (warning.mode == WarningMode::RpmCurve) {
        if (warning.rpm_curve_count == 0U) {
            text("No RPM points. Use ADD POINT.", 24, 110, &lv_font_montserrat_14, kInfoText);
        } else {
            if (selected_curve_point_ >= warning.rpm_curve_count) selected_curve_point_ = 0U;
            const auto& point = warning.rpm_curve[selected_curve_point_];
            std::snprintf(line, sizeof(line), "POINT %u/%u   RPM %.0f   WARN %.2f   CRIT %.2f",
                          static_cast<unsigned>(selected_curve_point_ + 1U),
                          static_cast<unsigned>(warning.rpm_curve_count),
                          static_cast<double>(point.rpm),
                          static_cast<double>(point.warning),
                          static_cast<double>(point.critical));
            text(line, 24, 110, &lv_font_montserrat_14);
        }
        std::snprintf(line, sizeof(line), "HYST %.2f   DELAY %u ms",
                      static_cast<double>(warning.hysteresis),
                      static_cast<unsigned>(warning.delay_ms));
        text(line, 24, 136, &lv_font_montserrat_12, kInfoText);
    } else {
        text("Alarm disabled for this parameter.", 24, 110, &lv_font_montserrat_14, kInfoText);
    }

    button("< PARAM", 24, 166, 104, warningPrevEvent);
    button("PARAM >", 136, 166, 104, warningNextEvent);
    button("MODE >", 248, 166, 104, warningModeEvent);
    button("WARN -", 360, 166, 82, warningWarnDownEvent);
    button("WARN +", 450, 166, 82, warningWarnUpEvent);
    button("CRIT -", 540, 166, 82, warningCriticalDownEvent);

    button("CRIT +", 24, 218, 82, warningCriticalUpEvent);
    button("HYST -", 114, 218, 82, warningHystDownEvent);
    button("HYST +", 204, 218, 82, warningHystUpEvent);
    button("DELAY -", 294, 218, 92, warningDelayDownEvent);
    button("DELAY +", 394, 218, 92, warningDelayUpEvent);

    if (warning.mode == WarningMode::RpmCurve) {
        button("POINT >", 494, 218, 90, warningCurvePointEvent);
        button("ADD", 24, 270, 70, warningCurveAddEvent);
        button("REMOVE", 102, 270, 90, warningCurveRemoveEvent);
        button("RPM -", 200, 270, 80, warningCurveRpmDownEvent);
        button("RPM +", 288, 270, 80, warningCurveRpmUpEvent);
    }
}

void SettingsOverlay::warningPrevEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->selected_warning_parameter_ = self->selected_warning_parameter_ == 0U
                                            ? AppConfig::kParameterCount - 1U
                                            : self->selected_warning_parameter_ - 1U;
    self->selected_curve_point_ = 0U;
    self->openWarnings();
}

void SettingsOverlay::warningNextEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->selected_warning_parameter_ = static_cast<uint16_t>(
        (self->selected_warning_parameter_ + 1U) % AppConfig::kParameterCount);
    self->selected_curve_point_ = 0U;
    self->openWarnings();
}

void SettingsOverlay::warningModeEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    WarningConfig* warning = self->selectedWarning();
    if (warning == nullptr) return;
    const uint8_t next = static_cast<uint8_t>((static_cast<uint8_t>(warning->mode) + 1U) % 5U);
    WarningEditorModel::setMode(*warning, static_cast<WarningMode>(next));
    self->selected_curve_point_ = 0U;
    self->save();
    self->openWarnings();
}

void SettingsOverlay::adjustWarningScalar(float delta, bool critical) {
    WarningConfig* warning = selectedWarning();
    if (warning == nullptr) return;

    if (warning->mode == WarningMode::High) {
        float w = warning->warning_threshold;
        float c = warning->critical_threshold;
        if (critical) c += delta; else w += delta;
        WarningEditorModel::setHighThresholds(*warning, w, c);
    } else if (warning->mode == WarningMode::Low) {
        float w = warning->warning_threshold;
        float c = warning->critical_threshold;
        if (critical) c += delta; else w += delta;
        WarningEditorModel::setLowThresholds(*warning, w, c);
    } else if (warning->mode == WarningMode::Range) {
        float wl = warning->warning_low;
        float wh = warning->warning_high;
        float cl = warning->critical_low;
        float ch = warning->critical_high;
        if (critical) {
            cl -= delta;
            ch += delta;
        } else {
            wl -= delta;
            wh += delta;
        }
        WarningEditorModel::setRangeThresholds(*warning, wl, wh, cl, ch);
    } else if (warning->mode == WarningMode::RpmCurve && warning->rpm_curve_count > 0U) {
        if (selected_curve_point_ >= warning->rpm_curve_count) selected_curve_point_ = 0U;
        auto point = warning->rpm_curve[selected_curve_point_];
        if (critical) point.critical += delta; else point.warning += delta;
        WarningEditorModel::updateRpmPoint(*warning, selected_curve_point_,
                                           point.rpm, point.warning, point.critical);
    }
}

void SettingsOverlay::warningWarnDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->adjustWarningScalar(-self->warningStep(), false);
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningWarnUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->adjustWarningScalar(self->warningStep(), false);
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningCriticalDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->adjustWarningScalar(-self->warningStep(), true);
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningCriticalUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    if (self == nullptr) return;
    self->adjustWarningScalar(self->warningStep(), true);
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningHystDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr) return;
    WarningEditorModel::setHysteresis(*warning, nonNegative(warning->hysteresis - self->warningStep()));
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningHystUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr) return;
    WarningEditorModel::setHysteresis(*warning, warning->hysteresis + self->warningStep());
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningDelayDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr) return;
    const uint32_t value = warning->delay_ms > 100U ? warning->delay_ms - 100U : 0U;
    WarningEditorModel::setDelayMs(*warning, value);
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningDelayUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr) return;
    const uint32_t value = warning->delay_ms >= 9900U ? 10000U : warning->delay_ms + 100U;
    WarningEditorModel::setDelayMs(*warning, value);
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningCurvePointEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr || warning->rpm_curve_count == 0U) return;
    self->selected_curve_point_ = static_cast<uint8_t>(
        (self->selected_curve_point_ + 1U) % warning->rpm_curve_count);
    self->openWarnings();
}

void SettingsOverlay::warningCurveAddEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr) return;
    const float rpm = warning->rpm_curve_count == 0U
                          ? 1000.0f
                          : warning->rpm_curve[warning->rpm_curve_count - 1U].rpm + 1000.0f;
    if (WarningEditorModel::addRpmPoint(*warning, rpm, 2.0f, 1.0f)) {
        self->selected_curve_point_ = static_cast<uint8_t>(warning->rpm_curve_count - 1U);
        self->save();
    }
    self->openWarnings();
}

void SettingsOverlay::warningCurveRemoveEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr || warning->rpm_curve_count == 0U) return;
    WarningEditorModel::removeRpmPoint(*warning, self->selected_curve_point_);
    if (self->selected_curve_point_ >= warning->rpm_curve_count && self->selected_curve_point_ > 0U) {
        --self->selected_curve_point_;
    }
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningCurveRpmDownEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr || warning->rpm_curve_count == 0U) return;
    auto point = warning->rpm_curve[self->selected_curve_point_];
    point.rpm = point.rpm >= 500.0f ? point.rpm - 500.0f : 0.0f;
    WarningEditorModel::updateRpmPoint(*warning, self->selected_curve_point_,
                                       point.rpm, point.warning, point.critical);
    self->save();
    self->openWarnings();
}

void SettingsOverlay::warningCurveRpmUpEvent(lv_event_t* event) {
    auto* self = static_cast<SettingsOverlay*>(lv_event_get_user_data(event));
    WarningConfig* warning = self == nullptr ? nullptr : self->selectedWarning();
    if (warning == nullptr || warning->rpm_curve_count == 0U) return;
    auto point = warning->rpm_curve[self->selected_curve_point_];
    point.rpm += 500.0f;
    if (point.rpm > 20000.0f) point.rpm = 20000.0f;
    WarningEditorModel::updateRpmPoint(*warning, self->selected_curve_point_,
                                       point.rpm, point.warning, point.critical);
    self->save();
    self->openWarnings();
}
