#include "racechrono_protocol.h"

#include <algorithm>
#include <cstring>

namespace {
constexpr std::size_t kEquationPayloadSize = 17U;

uint16_t readBigEndian16(const uint8_t* data) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(data[0]) << 8U) |
        static_cast<uint16_t>(data[1]));
}
}  // namespace

RaceChronoPacket RaceChronoProtocol::removeAll() {
    RaceChronoPacket packet;
    packet.bytes[0] = 0U;
    packet.size = 1U;
    return packet;
}

RaceChronoPacket RaceChronoProtocol::updateAll() {
    RaceChronoPacket packet;
    packet.bytes[0] = 4U;
    packet.size = 1U;
    return packet;
}

std::size_t RaceChronoProtocol::fragmentCount(const char* equation) {
    if (equation == nullptr) {
        return 0U;
    }
    const std::size_t length = std::strlen(equation);
    return length == 0U ? 0U
                        : (length + kEquationPayloadSize - 1U) /
                              kEquationPayloadSize;
}

RaceChronoPacket RaceChronoProtocol::addFragment(
    uint8_t monitor_id, const char* equation, std::size_t fragment_index) {
    RaceChronoPacket packet;
    const std::size_t fragment_count = fragmentCount(equation);
    if (fragment_count == 0U || fragment_index >= fragment_count ||
        fragment_index > 255U) {
        return packet;
    }

    const std::size_t length = std::strlen(equation);
    const std::size_t offset = fragment_index * kEquationPayloadSize;
    const std::size_t payload_size =
        std::min(kEquationPayloadSize, length - offset);

    packet.bytes[0] = fragment_index + 1U == fragment_count ? 3U : 2U;
    packet.bytes[1] = monitor_id;
    packet.bytes[2] = static_cast<uint8_t>(fragment_index);
    std::memcpy(packet.bytes.data() + 3U, equation + offset, payload_size);
    packet.size = static_cast<uint8_t>(3U + payload_size);
    return packet;
}

RaceChronoConfigResult RaceChronoProtocol::decodeConfigResult(
    const uint8_t* data, std::size_t size) {
    RaceChronoConfigResult result;
    if (data == nullptr || size < 2U) {
        return result;
    }

    result.monitor_id = data[1];
    if (data[0] == 0U && size == 2U) {
        result.type = RaceChronoConfigResultType::Success;
    } else if (data[0] == 1U && size == 2U) {
        result.type = RaceChronoConfigResultType::PayloadOutOfSequence;
    } else if (data[0] == 2U && size == 8U) {
        result.type = RaceChronoConfigResultType::EquationException;
        result.exception_type = readBigEndian16(data + 2U);
        result.exception_position = readBigEndian16(data + 4U);
        result.exception_length = readBigEndian16(data + 6U);
    }
    return result;
}

RaceChronoValueBatch RaceChronoProtocol::decodeValues(const uint8_t* data,
                                                       std::size_t size) {
    RaceChronoValueBatch batch;
    if (data == nullptr || size == 0U) {
        batch.error = RaceChronoDecodeError::Empty;
        return batch;
    }
    if (size > 20U) {
        batch.error = size % 5U == 0U
                          ? RaceChronoDecodeError::TooManyValues
                          : RaceChronoDecodeError::InvalidLength;
        return batch;
    }
    if (size % 5U != 0U) {
        batch.error = RaceChronoDecodeError::InvalidLength;
        return batch;
    }

    batch.count = static_cast<uint8_t>(size / 5U);
    for (std::size_t index = 0U; index < batch.count; ++index) {
        const std::size_t offset = index * 5U;
        const uint32_t encoded =
            (static_cast<uint32_t>(data[offset + 1U]) << 24U) |
            (static_cast<uint32_t>(data[offset + 2U]) << 16U) |
            (static_cast<uint32_t>(data[offset + 3U]) << 8U) |
            static_cast<uint32_t>(data[offset + 4U]);
        batch.values[index] =
            RaceChronoDecodedValue{data[offset], static_cast<int32_t>(encoded)};
    }
    return batch;
}
