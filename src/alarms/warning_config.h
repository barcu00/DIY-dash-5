#pragma once

#include <array>
#include <cstdint>

enum class WarningMode : uint8_t {
    Off = 0,
    Low,
    High,
    Range,
    RpmCurve,
};

enum class AlarmState : uint8_t {
    Normal = 0,
    Warning = 1,
    Critical = 2,
    Invalid = 3,
};

struct RpmWarningPoint {
    float rpm = 0.0f;
    float warning = 0.0f;
    float critical = 0.0f;
};

struct WarningConfig {
    static constexpr uint8_t kMaxRpmCurvePoints = 8;

    WarningMode mode = WarningMode::Off;

    float warning_threshold = 0.0f;
    float critical_threshold = 0.0f;

    float warning_low = 0.0f;
    float warning_high = 0.0f;
    float critical_low = 0.0f;
    float critical_high = 0.0f;

    float hysteresis = 0.0f;
    uint32_t delay_ms = 0U;

    std::array<RpmWarningPoint, kMaxRpmCurvePoints> rpm_curve{};
    uint8_t rpm_curve_count = 0U;
};
