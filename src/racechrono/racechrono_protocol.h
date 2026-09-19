#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct RaceChronoPacket {
    std::array<uint8_t, 20U> bytes{};
    uint8_t size = 0U;
};

struct RaceChronoDecodedValue {
    uint8_t monitor_id = 0U;
    int32_t raw = 0;
};

enum class RaceChronoDecodeError : uint8_t {
    None,
    Empty,
    InvalidLength,
    TooManyValues,
};

struct RaceChronoValueBatch {
    std::array<RaceChronoDecodedValue, 4U> values{};
    uint8_t count = 0U;
    RaceChronoDecodeError error = RaceChronoDecodeError::None;
};

enum class RaceChronoConfigResultType : uint8_t {
    Invalid,
    Success,
    PayloadOutOfSequence,
    EquationException,
};

struct RaceChronoConfigResult {
    RaceChronoConfigResultType type = RaceChronoConfigResultType::Invalid;
    uint8_t monitor_id = 0U;
    uint16_t exception_type = 0U;
    uint16_t exception_position = 0U;
    uint16_t exception_length = 0U;
};

class RaceChronoProtocol {
public:
    static RaceChronoPacket removeAll();
    static RaceChronoPacket updateAll();
    static std::size_t fragmentCount(const char* equation);
    static RaceChronoPacket addFragment(uint8_t monitor_id,
                                        const char* equation,
                                        std::size_t fragment_index);
    static RaceChronoConfigResult decodeConfigResult(const uint8_t* data,
                                                       std::size_t size);
    static RaceChronoValueBatch decodeValues(const uint8_t* data,
                                              std::size_t size);
};
