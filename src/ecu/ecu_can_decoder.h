#pragma once

#include <cstddef>
#include <cstdint>

#include "can/can_frame.h"
#include "telemetry/vehicle_state.h"

enum class ByteOrder : uint8_t {
    Little,
    Big,
};

enum class RawType : uint8_t {
    Unsigned8,
    Signed8,
    Unsigned16,
    Signed16,
    Unsigned32,
    Signed32,
};

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

    bool decode(const CanFrame& frame, VehicleState& state, uint32_t now_ms) const;
    std::size_t definitionCount() const;
    uint32_t timeoutFor(VehicleSignal signal) const;

private:
    static std::size_t rawWidth(RawType type);
    static uint32_t readUnsigned(const uint8_t* data, std::size_t width,
                                 ByteOrder order);
    static float readRaw(const uint8_t* data, RawType type, ByteOrder order);

    const SignalDefinition* definitions_ = nullptr;
    std::size_t count_ = 0U;
};
