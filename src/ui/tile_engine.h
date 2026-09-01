#pragma once

#include <array>
#include <cstdint>

#include "settings/app_config.h"

class TileEngine {
public:
    static constexpr uint8_t kNoSlot = 0xFFU;

    static uint8_t visibleOrder(const std::array<TileConfig, AppConfig::kTileCount>& tiles,
                                std::array<uint8_t, AppConfig::kTileCount>& order);
    static uint8_t compactSlot(const std::array<TileConfig, AppConfig::kTileCount>& tiles,
                               uint8_t tile_index);
    static float lambdaToAfr(float lambda, float stoich_afr);
    static float presentValue(const TileConfig& tile, float raw_value, float stoich_afr);
};
