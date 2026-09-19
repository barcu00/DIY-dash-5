#pragma once

#include <array>
#include <cstdint>

#include "racechrono_channel_catalog.h"
#include "racechrono_protocol.h"
#include "telemetry/vehicle_state.h"

enum class RaceChronoChannelState : uint8_t {
    NoData,
    Active,
    Error,
};

struct RaceChronoRuntimeStatus {
    uint32_t value_packets = 0U;
    uint32_t malformed_packets = 0U;
    uint32_t unknown_monitor_ids = 0U;
    uint32_t last_packet_age_ms = 0U;
    uint8_t configured_channels = 0U;
    uint8_t active_channels = 0U;
    uint8_t error_channels = 0U;
    uint8_t satellites = 0U;
    uint8_t gps_fix_type = 0U;
    float gps_accuracy = 0.0f;
    bool has_valid_packet = false;
};

class RaceChronoTelemetry {
public:
    bool acceptRaw(uint8_t monitor_id, int32_t raw, uint32_t now_ms);
    bool acceptBatch(const RaceChronoValueBatch& batch, uint32_t now_ms);
    void updateStale(uint32_t now_ms);
    void invalidateAll();
    const SignalValue& get(ParameterId id) const;
    RaceChronoChannelState channelState(ParameterId id) const;
    RaceChronoRuntimeStatus snapshotStatus(uint32_t now_ms) const;
    void markConfigured(ParameterId id);
    void markError(ParameterId id);

private:
    static std::size_t indexOf(ParameterId id);

    std::array<SignalValue, 33U> values_{};
    std::array<RaceChronoChannelState, 33U> states_{};
    std::array<bool, 33U> configured_{};
    uint32_t value_packets_ = 0U;
    uint32_t malformed_packets_ = 0U;
    uint32_t unknown_monitor_ids_ = 0U;
    uint32_t last_valid_packet_ms_ = 0U;
    bool has_valid_packet_ = false;
};
