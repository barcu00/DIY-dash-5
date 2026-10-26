#pragma once

#include <cstdint>

#include "settings/app_config.h"

struct GaugeRange {
    float minimum = 0.0f;
    float maximum = 100.0f;
    // Fraction (0..1000) where the red zone starts, or > 1000 for none.
    uint16_t red_from_permille = 1001U;
    // Low-side red zone end (oil pressure "below" warnings), 0 for none.
    uint16_t red_below_permille = 0U;
};

// Scale of a mini needle gauge: the temperature bar range for temperatures,
// otherwise a sensible range for the native unit. The tile warning threshold
// marks the red zone on the side it warns about.
GaugeRange gaugeRange(const TileConfig& tile);
uint16_t gaugeFill(float native_value, const GaugeRange& range);
