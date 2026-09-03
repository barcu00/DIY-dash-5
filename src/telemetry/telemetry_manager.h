#pragma once

#include <array>
#include <cstdint>

#include "can/can_status.h"
#include "ecu/ecu_can_decoder.h"
#include "telemetry/mock_telemetry.h"

class TelemetryManager {
public:
    TelemetryManager(const EcuCanDecoder& decoder, bool demo_enabled,
                     uint32_t can_timeout_ms);

    void setCanInitialized(bool initialized, uint32_t now_ms);
    bool accept(const CanFrame& frame, uint32_t now_ms);
    void update(uint32_t now_ms);

    const VehicleState& state() const;
    CanStatus canStatus() const;
    bool demoActive() const;
    std::size_t mappingCount() const;

private:
    const EcuCanDecoder& decoder_;
    MockTelemetry demo_;
    VehicleState can_state_{};
    VehicleState empty_state_{};
    std::array<uint32_t, VehicleState::kSignalCount> timeouts_{};
    bool demo_enabled_ = false;
    bool demo_active_ = false;
    bool can_initialized_ = true;
    bool has_valid_frame_ = false;
    uint32_t can_timeout_ms_ = 0U;
    uint32_t started_ms_ = 0U;
    uint32_t last_valid_frame_ms_ = 0U;
    CanStatus can_status_ = CanStatus::Waiting;
};
