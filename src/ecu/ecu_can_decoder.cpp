#include "ecu_can_decoder.h"

#include <cstring>

EcuCanDecoder::EcuCanDecoder(const SignalDefinition* definitions,
                             std::size_t count)
    : definitions_(definitions), count_(definitions == nullptr ? 0U : count) {}

bool EcuCanDecoder::decode(const CanFrame& frame, VehicleState& state,
                           uint32_t now_ms) const {
    if (frame.remote || frame.dlc > sizeof(frame.data)) {
        return false;
    }

    bool decoded = false;
    for (std::size_t i = 0; i < count_; ++i) {
        const SignalDefinition& definition = definitions_[i];
        if (definition.can_id != frame.id || definition.extended != frame.extended) {
            continue;
        }

        const std::size_t width = rawWidth(definition.raw_type);
        if (width == 0U || definition.byte_offset > frame.dlc ||
            width > static_cast<std::size_t>(frame.dlc - definition.byte_offset)) {
            continue;
        }

        const float raw = readRaw(frame.data + definition.byte_offset,
                                  definition.raw_type, definition.byte_order);
        state.set(definition.signal, raw * definition.scale + definition.bias,
                  now_ms);
        decoded = true;
    }
    return decoded;
}

std::size_t EcuCanDecoder::definitionCount() const {
    return count_;
}

uint32_t EcuCanDecoder::timeoutFor(VehicleSignal signal) const {
    uint32_t timeout_ms = 0U;
    for (std::size_t i = 0; i < count_; ++i) {
        if (definitions_[i].signal == signal) {
            timeout_ms = definitions_[i].timeout_ms;
        }
    }
    return timeout_ms;
}

std::size_t EcuCanDecoder::rawWidth(RawType type) {
    switch (type) {
        case RawType::Unsigned8:
        case RawType::Signed8:
            return 1U;
        case RawType::Unsigned16:
        case RawType::Signed16:
            return 2U;
        case RawType::Unsigned32:
        case RawType::Signed32:
            return 4U;
    }
    return 0U;
}

uint32_t EcuCanDecoder::readUnsigned(const uint8_t* data, std::size_t width,
                                     ByteOrder order) {
    uint32_t value = 0U;
    for (std::size_t i = 0; i < width; ++i) {
        const std::size_t source = order == ByteOrder::Little ? i : width - 1U - i;
        value |= static_cast<uint32_t>(data[source]) << (8U * i);
    }
    return value;
}

float EcuCanDecoder::readRaw(const uint8_t* data, RawType type,
                             ByteOrder order) {
    const uint32_t value = readUnsigned(data, rawWidth(type), order);
    switch (type) {
        case RawType::Unsigned8:
            return static_cast<float>(static_cast<uint8_t>(value));
        case RawType::Signed8:
            return static_cast<float>(static_cast<int8_t>(value));
        case RawType::Unsigned16:
            return static_cast<float>(static_cast<uint16_t>(value));
        case RawType::Signed16:
            return static_cast<float>(static_cast<int16_t>(value));
        case RawType::Unsigned32:
            return static_cast<float>(value);
        case RawType::Signed32: {
            int32_t signed_value = 0;
            std::memcpy(&signed_value, &value, sizeof(signed_value));
            return static_cast<float>(signed_value);
        }
    }
    return 0.0f;
}
