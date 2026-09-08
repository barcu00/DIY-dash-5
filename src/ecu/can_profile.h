#pragma once

#include <cstddef>
#include <cstdint>

#include "telemetry/parameter_id.h"

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

enum class CanProfileVerification : uint8_t {
    Verified,
    Experimental,
};

struct CanSignalDefinition {
    VehicleSignal signal;
    uint8_t byte_offset;
    RawType raw_type;
    ByteOrder byte_order;
    float scale;
    float bias;
    float minimum_native;
    float maximum_native;
    uint32_t timeout_ms;
};

struct CanFrameDefinition {
    uint32_t can_id;
    bool extended;
    uint8_t expected_dlc;
    uint8_t discriminator_offset;
    uint16_t discriminator_mask;
    uint16_t discriminator_value;
    const CanSignalDefinition* signals;
    std::size_t signal_count;
};

struct CanProfile {
    const char* id;
    const char* name;
    const char* family;
    const char* protocol_version;
    CanProfileVerification verification;
    uint32_t default_bitrate;
    const char* source_url;
    const char* source_revision;
    const char* provenance_note;
    const CanFrameDefinition* frames;
    std::size_t frame_count;
};
