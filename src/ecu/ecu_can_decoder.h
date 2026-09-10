#pragma once

#include <cstddef>
#include <cstdint>

#include "can/can_frame.h"
#include "ecu/can_profile.h"
#include "telemetry/vehicle_state.h"

// Retained for small custom/legacy decoder users. Compiled ECU profiles use
// CanFrameDefinition so a whole frame can be validated and committed atomically.
struct SignalDefinition {
    uint32_t can_id;
    bool extended;
    uint8_t byte_offset;
    ByteOrder byte_order;
    RawType raw_type;
    float scale;
    float bias;
    VehicleSignal signal;
    const char* unit;
    uint32_t timeout_ms;
};

class EcuCanDecoder {
public:
    EcuCanDecoder(const SignalDefinition* definitions, std::size_t count);
    explicit EcuCanDecoder(const CanProfile* profile);

    bool decode(const CanFrame& frame, VehicleState& state, uint32_t now_ms) const;
    void selectProfile(const CanProfile* profile);
    const CanProfile* profile() const;
    std::size_t definitionCount() const;
    uint32_t timeoutFor(VehicleSignal signal) const;

private:
    static std::size_t rawWidth(RawType type);
    static uint32_t readUnsigned(const uint8_t* data, std::size_t width,
                                 ByteOrder order);
    static float readRaw(const uint8_t* data, RawType type, ByteOrder order);

    const SignalDefinition* definitions_ = nullptr;
    std::size_t count_ = 0U;
    const CanProfile* profile_ = nullptr;
};
