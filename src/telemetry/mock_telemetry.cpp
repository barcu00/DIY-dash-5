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
    state_.reset(DataSource::Demo);
    state_.set(VehicleSignal::Rpm, 900.0f, 0U);
    state_.set(VehicleSignal::Gear, 1.0f, 0U);
    state_.set(VehicleSignal::Speed, 0.0f, 0U);
    state_.set(VehicleSignal::Map, 0.40f, 0U);
    state_.set(VehicleSignal::Lambda, 1.00f, 0U);
    state_.set(VehicleSignal::Clt, 85.0f, 0U);
    state_.set(VehicleSignal::Iat, 30.0f, 0U);
    state_.set(VehicleSignal::OilPressure, 1.40f, 0U);
    state_.set(VehicleSignal::OilTemperature, 80.0f, 0U);
    state_.set(VehicleSignal::FuelPressure, 3.50f, 0U);
    state_.set(VehicleSignal::BatteryVoltage, 13.80f, 0U);
    state_.set(VehicleSignal::Tps, 0.0f, 0U);
}

void MockTelemetry::update(uint32_t elapsed_ms) {
    const float throttle_wave = wave(elapsed_ms, 8.0f);
    const float load_wave = wave(elapsed_ms, 11.0f, 0.8f);
    const float thermal_wave = wave(elapsed_ms, 37.0f, 1.4f);

    const float tps = lerp(0.0f, 100.0f, throttle_wave);
    const float rpm = static_cast<float>(std::lround(
        lerp(900.0f, 7800.0f, throttle_wave)));
    const float speed = lerp(0.0f, 190.0f,
                             wave(elapsed_ms, 24.0f, -1.0f));
    const int gear = std::clamp(1 + static_cast<int>(speed / 36.0f), 1, 6);

    state_.reset(DataSource::Demo);
    state_.set(VehicleSignal::Tps, tps, elapsed_ms);
    state_.set(VehicleSignal::Rpm, rpm, elapsed_ms);
    state_.set(VehicleSignal::Speed, speed, elapsed_ms);
    state_.set(VehicleSignal::Gear, static_cast<float>(gear), elapsed_ms);
    state_.set(VehicleSignal::Map, lerp(0.35f, 1.50f, load_wave), elapsed_ms);
    state_.set(VehicleSignal::Lambda,
               lerp(1.05f, 0.78f, 0.65f * throttle_wave + 0.35f * load_wave),
               elapsed_ms);
    state_.set(VehicleSignal::Clt, lerp(80.0f, 103.0f, thermal_wave), elapsed_ms);
    state_.set(VehicleSignal::Iat, lerp(25.0f, 55.0f, load_wave), elapsed_ms);
    state_.set(VehicleSignal::OilPressure,
               lerp(1.20f, 5.80f, (rpm - 900.0f) / 6900.0f), elapsed_ms);
    state_.set(VehicleSignal::OilTemperature,
               lerp(75.0f, 120.0f, wave(elapsed_ms, 49.0f, 0.3f)), elapsed_ms);
    state_.set(VehicleSignal::FuelPressure, lerp(3.0f, 4.5f, load_wave), elapsed_ms);
    state_.set(VehicleSignal::BatteryVoltage,
               lerp(12.8f, 14.4f, wave(elapsed_ms, 13.0f, 2.0f)), elapsed_ms);
}

const VehicleState& MockTelemetry::state() const {
    return state_;
}
