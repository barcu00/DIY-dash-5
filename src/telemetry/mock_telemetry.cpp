#include "mock_telemetry.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;

float wave(uint32_t elapsed_ms, float period_s, float phase = 0.0f) {
    const float seconds = static_cast<float>(elapsed_ms) / 1000.0f;
    const float angle = (seconds / period_s) * 2.0f * kPi + phase;
    return 0.5f + 0.5f * std::sin(angle);
}

float lerp(float low, float high, float t) {
    return low + (high - low) * std::clamp(t, 0.0f, 1.0f);
}
}  // namespace

MockTelemetry::MockTelemetry() {
    reset();
}

void MockTelemetry::reset() {
    state_.rpm = 900;
    state_.gear = 1;
    state_.speed_kph = 0.0f;
    state_.map_bar = 0.40f;
    state_.lambda = 1.00f;
    state_.clt_c = 85.0f;
    state_.iat_c = 30.0f;
    state_.oil_pressure_bar = 1.40f;
    state_.oil_temp_c = 80.0f;
    state_.fuel_pressure_bar = 3.50f;
    state_.battery_v = 13.80f;
    state_.tps_percent = 0.0f;
}

void MockTelemetry::update(uint32_t elapsed_ms) {
    const float throttle_wave = wave(elapsed_ms, 8.0f);
    const float load_wave = wave(elapsed_ms, 11.0f, 0.8f);
    const float thermal_wave = wave(elapsed_ms, 37.0f, 1.4f);

    state_.tps_percent = lerp(0.0f, 100.0f, throttle_wave);
    state_.rpm = static_cast<uint16_t>(std::lround(lerp(900.0f, 7800.0f, throttle_wave)));
    state_.speed_kph = lerp(0.0f, 190.0f, wave(elapsed_ms, 24.0f, -1.0f));

    const int calculated_gear = 1 + static_cast<int>(state_.speed_kph / 36.0f);
    state_.gear = static_cast<int8_t>(std::clamp(calculated_gear, 1, 6));

    state_.map_bar = lerp(0.35f, 1.50f, load_wave);
    state_.lambda = lerp(1.05f, 0.78f, 0.65f * throttle_wave + 0.35f * load_wave);
    state_.clt_c = lerp(80.0f, 103.0f, thermal_wave);
    state_.iat_c = lerp(25.0f, 55.0f, load_wave);
    state_.oil_pressure_bar = lerp(1.20f, 5.80f, static_cast<float>(state_.rpm - 900) / 6900.0f);
    state_.oil_temp_c = lerp(75.0f, 120.0f, wave(elapsed_ms, 49.0f, 0.3f));
    state_.fuel_pressure_bar = lerp(3.0f, 4.5f, load_wave);
    state_.battery_v = lerp(12.8f, 14.4f, wave(elapsed_ms, 13.0f, 2.0f));
}

const VehicleState& MockTelemetry::state() const {
    return state_;
}
