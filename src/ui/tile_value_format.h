#pragma once

#include <cstddef>
#include <cstdint>

#include "telemetry/parameter_id.h"

enum class DeltaTone : uint8_t {
    Neutral,
    Better,  // negative lap delta
    Worse,   // positive lap delta
};

// Formats a presented value for a tile. Times of a minute or more are shown
// as m:ss.d (lap times), lap deltas always carry a sign.
void formatTileValue(ParameterId id, float presented, uint8_t decimals,
                     char* output, std::size_t capacity);
DeltaTone deltaTone(ParameterId id, float presented);
