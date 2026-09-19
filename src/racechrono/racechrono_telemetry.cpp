#include "racechrono_telemetry.h"

#include <limits>

namespace {
constexpr std::size_t kInvalidIndex = 33U;
const SignalValue kInvalidValue{};
}  // namespace

std::size_t RaceChronoTelemetry::indexOf(ParameterId id) {
    if (!isRaceChronoParameter(id)) {
        return kInvalidIndex;
    }
    return static_cast<uint8_t>(id) -
           static_cast<uint8_t>(ParameterId::RcLapNumber);
}

bool RaceChronoTelemetry::acceptRaw(uint8_t monitor_id, int32_t raw,
                                    uint32_t now_ms) {
    const RaceChronoChannelDescriptor* descriptor =
        raceChronoChannelByMonitorId(monitor_id);
    if (descriptor == nullptr) {
        ++unknown_monitor_ids_;
        return false;
    }

    const std::size_t index = indexOf(descriptor->parameter);
    if (descriptor->encoding ==
            RaceChronoValueEncoding::CoordinateDegreesTimes6000000 &&
        raw == std::numeric_limits<int32_t>::max()) {
        values_[index].valid = false;
        states_[index] = RaceChronoChannelState::NoData;
        return false;
    }

    values_[index] = SignalValue{
        static_cast<float>(raw) * descriptor->raw_to_native, now_ms, true};
    states_[index] = RaceChronoChannelState::Active;
    return true;
}

bool RaceChronoTelemetry::acceptBatch(const RaceChronoValueBatch& batch,
                                      uint32_t now_ms) {
    if (batch.error != RaceChronoDecodeError::None ||
        batch.count > batch.values.size()) {
        ++malformed_packets_;
        return false;
    }

    ++value_packets_;
    bool accepted = false;
    for (std::size_t index = 0U; index < batch.count; ++index) {
        accepted = acceptRaw(batch.values[index].monitor_id,
                             batch.values[index].raw, now_ms) || accepted;
    }
    if (accepted) {
        last_valid_packet_ms_ = now_ms;
        has_valid_packet_ = true;
    }
    return accepted;
}

void RaceChronoTelemetry::updateStale(uint32_t now_ms) {
    for (std::size_t index = 0U; index < values_.size(); ++index) {
        if (!values_[index].valid) {
            continue;
        }
        const auto& descriptor = raceChronoChannelAt(index);
        if (now_ms - values_[index].updated_ms > descriptor.stale_ms) {
            values_[index].valid = false;
            states_[index] = RaceChronoChannelState::NoData;
        }
    }
}

void RaceChronoTelemetry::invalidateAll() {
    values_.fill(SignalValue{});
    states_.fill(RaceChronoChannelState::NoData);
    configured_.fill(false);
    has_valid_packet_ = false;
    last_valid_packet_ms_ = 0U;
}

const SignalValue& RaceChronoTelemetry::get(ParameterId id) const {
    const std::size_t index = indexOf(id);
    return index < values_.size() ? values_[index] : kInvalidValue;
}

RaceChronoChannelState RaceChronoTelemetry::channelState(ParameterId id) const {
    const std::size_t index = indexOf(id);
    return index < states_.size() ? states_[index]
                                  : RaceChronoChannelState::NoData;
}

RaceChronoRuntimeStatus RaceChronoTelemetry::snapshotStatus(
    uint32_t now_ms) const {
    RaceChronoRuntimeStatus status;
    status.value_packets = value_packets_;
    status.malformed_packets = malformed_packets_;
    status.unknown_monitor_ids = unknown_monitor_ids_;
    status.has_valid_packet = has_valid_packet_;
    status.last_packet_age_ms =
        has_valid_packet_ ? now_ms - last_valid_packet_ms_ : 0U;
    for (std::size_t index = 0U; index < states_.size(); ++index) {
        status.configured_channels += configured_[index] ? 1U : 0U;
        status.active_channels +=
            states_[index] == RaceChronoChannelState::Active ? 1U : 0U;
        status.error_channels +=
            states_[index] == RaceChronoChannelState::Error ? 1U : 0U;
    }
    const auto& satellites = get(ParameterId::RcSatellites);
    const auto& fix = get(ParameterId::RcFixType);
    const auto& accuracy = get(ParameterId::RcAccuracy);
    status.satellites =
        satellites.valid ? static_cast<uint8_t>(satellites.value) : 0U;
    status.gps_fix_type = fix.valid ? static_cast<uint8_t>(fix.value) : 0U;
    status.gps_accuracy = accuracy.valid ? accuracy.value : 0.0f;
    return status;
}

void RaceChronoTelemetry::markConfigured(ParameterId id) {
    const std::size_t index = indexOf(id);
    if (index < configured_.size()) {
        configured_[index] = true;
        if (states_[index] == RaceChronoChannelState::Error) {
            states_[index] = RaceChronoChannelState::NoData;
        }
    }
}

void RaceChronoTelemetry::markError(ParameterId id) {
    const std::size_t index = indexOf(id);
    if (index < states_.size()) {
        values_[index].valid = false;
        configured_[index] = false;
        states_[index] = RaceChronoChannelState::Error;
    }
}
