#pragma once

#include <cstddef>
#include <cstdint>

#include "telemetry/parameter_registry.h"

class EcumasterDecoder {
public:
    explicit EcumasterDecoder(ParameterRegistry& registry);

    void setBaseId(uint16_t base_id);
    uint16_t baseId() const;

    bool decode(uint32_t can_id, const uint8_t* data, std::size_t dlc, uint32_t now_ms);

private:
    ParameterRegistry& registry_;
    uint16_t base_id_ = 0x600;

    static uint16_t u16le(const uint8_t* data);
    static int16_t s16le(const uint8_t* data);
    static float bitValue(uint32_t value, uint8_t bit);

    void decode600(const uint8_t* d, uint32_t now_ms);
    void decode601(const uint8_t* d, uint32_t now_ms);
    void decode602(const uint8_t* d, uint32_t now_ms);
    void decode603(const uint8_t* d, uint32_t now_ms);
    void decode604(const uint8_t* d, uint32_t now_ms);
    void decode605(const uint8_t* d, uint32_t now_ms);
    void decode606(const uint8_t* d, uint32_t now_ms);
    void decode607(const uint8_t* d, uint32_t now_ms);
};
