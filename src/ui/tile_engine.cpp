#include "tile_engine.h"

uint8_t TileEngine::visibleOrder(
    const std::array<TileConfig, AppConfig::kTileCount>& tiles,
    std::array<uint8_t, AppConfig::kTileCount>& order) {
    uint8_t count = 0U;
    for (uint8_t i = 0U; i < AppConfig::kTileCount; ++i) {
        if (!tiles[i].visible) continue;
        order[count++] = i;
    }
    return count;
}

uint8_t TileEngine::compactSlot(
    const std::array<TileConfig, AppConfig::kTileCount>& tiles,
    uint8_t tile_index) {
    if (tile_index >= AppConfig::kTileCount || !tiles[tile_index].visible) {
        return kNoSlot;
    }

    uint8_t slot = 0U;
    for (uint8_t i = 0U; i < tile_index; ++i) {
        if (tiles[i].visible) ++slot;
    }
    return slot;
}

float TileEngine::lambdaToAfr(float lambda, float stoich_afr) {
    return lambda * stoich_afr;
}

float TileEngine::presentValue(const TileConfig& tile, float raw_value, float stoich_afr) {
    const bool lambda_parameter =
        tile.parameter == ParameterId::Lambda || tile.parameter == ParameterId::LambdaTarget;
    if (lambda_parameter && tile.value_format == ValueFormatMode::Afr) {
        return lambdaToAfr(raw_value, stoich_afr);
    }
    return raw_value;
}
