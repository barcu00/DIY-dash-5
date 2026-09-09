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

float smoothStep(float value) {
    const float t = std::clamp(value, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float demoEngineCycle(uint32_t elapsed_ms) {
    constexpr uint32_t kCycleMs = 10000U;
    constexpr uint32_t kRiseEndMs = 4000U;
    constexpr uint32_t kHoldEndMs = 6000U;
    const uint32_t phase_ms = elapsed_ms % kCycleMs;
    if (phase_ms < kRiseEndMs) {
        return smoothStep(static_cast<float>(phase_ms) /
                          static_cast<float>(kRiseEndMs));
    }
    if (phase_ms < kHoldEndMs) {
        return 1.0f;
    }
    return 1.0f - smoothStep(
        static_cast<float>(phase_ms - kHoldEndMs) /
        static_cast<float>(kCycleMs - kHoldEndMs));
}

void setDemoFlags(VehicleState& state, uint32_t elapsed_ms) {
    const std::size_t first =
        static_cast<std::size_t>(ParameterId::IgnitionOn);
    for (std::size_t index = first; index < parameterCount(); ++index) {
        const bool active = ((elapsed_ms / 1000U) + index) % 4U == 0U;
        state.set(static_cast<VehicleSignal>(index), active ? 1.0f : 0.0f,
                  elapsed_ms);
    }
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
    state_.set(VehicleSignal::BarometricPressure, 1.013f, 0U);
    state_.set(VehicleSignal::BoostTarget, 0.40f, 0U);
    state_.set(VehicleSignal::CoolantPressure, 1.10f, 0U);
    state_.set(VehicleSignal::FuelTemperature, 28.0f, 0U);
    state_.set(VehicleSignal::EthanolContent, 10.0f, 0U);
    state_.set(VehicleSignal::Lambda2, 1.00f, 0U);
    state_.set(VehicleSignal::IgnitionTiming, 10.0f, 0U);
    state_.set(VehicleSignal::InjectorDuty, 5.0f, 0U);
    state_.set(VehicleSignal::InjectorPulseWidth, 1.8f, 0U);
    state_.set(VehicleSignal::AcceleratorPosition, 0.0f, 0U);
    state_.set(VehicleSignal::MassAirFlow, 4.0f, 0U);
    for (uint8_t i = 0U; i < 8U; ++i) {
        state_.set(static_cast<VehicleSignal>(
                       static_cast<uint8_t>(VehicleSignal::Egt1) + i),
                   450.0f + 5.0f * i, 0U);
    }
    state_.set(VehicleSignal::WheelSpeedLf, 0.0f, 0U);
    state_.set(VehicleSignal::WheelSpeedLr, 0.0f, 0U);
    state_.set(VehicleSignal::WheelSpeedRf, 0.0f, 0U);
    state_.set(VehicleSignal::WheelSpeedRr, 0.0f, 0U);
    setDemoFlags(state_, 0U);
}

void MockTelemetry::update(uint32_t elapsed_ms) {
    const float throttle_wave = demoEngineCycle(elapsed_ms);
    const float load_wave = wave(elapsed_ms, 11.0f, 0.8f);
    const float thermal_wave = wave(elapsed_ms, 37.0f, 1.4f);

    const float tps = lerp(0.0f, 100.0f, throttle_wave);
    const float rpm = lerp(900.0f, 7800.0f, throttle_wave);
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
    state_.set(VehicleSignal::BarometricPressure,
               lerp(0.98f, 1.03f, thermal_wave), elapsed_ms);
    state_.set(VehicleSignal::BoostTarget,
               lerp(0.4f, 1.8f, throttle_wave), elapsed_ms);
    state_.set(VehicleSignal::CoolantPressure,
               lerp(0.9f, 1.8f, thermal_wave), elapsed_ms);
    state_.set(VehicleSignal::FuelTemperature,
               lerp(25.0f, 60.0f, thermal_wave), elapsed_ms);
    state_.set(VehicleSignal::EthanolContent, 10.0f, elapsed_ms);
    state_.set(VehicleSignal::Lambda2,
               lerp(1.04f, 0.80f, throttle_wave), elapsed_ms);
    state_.set(VehicleSignal::IgnitionTiming,
               lerp(-5.0f, 35.0f, load_wave), elapsed_ms);
    state_.set(VehicleSignal::InjectorDuty,
               lerp(5.0f, 85.0f, throttle_wave), elapsed_ms);
    state_.set(VehicleSignal::InjectorPulseWidth,
               lerp(1.5f, 14.0f, throttle_wave), elapsed_ms);
    state_.set(VehicleSignal::AcceleratorPosition, tps, elapsed_ms);
    state_.set(VehicleSignal::MassAirFlow,
               lerp(4.0f, 320.0f, load_wave), elapsed_ms);
    for (uint8_t i = 0U; i < 8U; ++i) {
        state_.set(static_cast<VehicleSignal>(
                       static_cast<uint8_t>(VehicleSignal::Egt1) + i),
                   lerp(450.0f + 3.0f * i, 950.0f + 3.0f * i,
                        throttle_wave),
                   elapsed_ms);
    }
    state_.set(VehicleSignal::WheelSpeedLf, speed, elapsed_ms);
    state_.set(VehicleSignal::WheelSpeedLr,
               std::clamp(speed * 0.995f, 0.0f, 200.0f), elapsed_ms);
    state_.set(VehicleSignal::WheelSpeedRf,
               std::clamp(speed * 1.002f, 0.0f, 200.0f), elapsed_ms);
    state_.set(VehicleSignal::WheelSpeedRr,
               std::clamp(speed * 0.998f, 0.0f, 200.0f), elapsed_ms);
    setDemoFlags(state_, elapsed_ms);
}

const VehicleState& MockTelemetry::state() const {
    return state_;
}
