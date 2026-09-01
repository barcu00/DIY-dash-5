#include "app_config.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr bool supportedBitrate(uint32_t bitrate) {
    return bitrate == 125000U || bitrate == 250000U ||
           bitrate == 500000U || bitrate == 1000000U;
}

TileConfig tile(ParameterId parameter, uint8_t decimals, bool visible = true) {
    TileConfig cfg{};
    cfg.parameter = parameter;
    cfg.visible = visible;
    cfg.decimals = decimals;
    return cfg;
}

void validateTiles(std::array<TileConfig, AppConfig::kTileCount>& tiles) {
    for (auto& tile_cfg : tiles) {
        if (parameterIndex(tile_cfg.parameter) >= AppConfig::kParameterCount) {
            tile_cfg.parameter = ParameterId::Rpm;
        }
        tile_cfg.decimals = std::min<uint8_t>(tile_cfg.decimals, 3U);
        tile_cfg.custom_label.back() = '\0';
        const uint8_t format = static_cast<uint8_t>(tile_cfg.value_format);
        if (format > static_cast<uint8_t>(ValueFormatMode::Afr)) {
            tile_cfg.value_format = ValueFormatMode::Native;
        }
    }
}
}

AppConfig AppConfig::defaults() {
    AppConfig cfg{};
    cfg.schema_version = kSchemaVersion;
    cfg.data_source = DataSource::Ecumaster;
    cfg.can_bitrate = 1000000U;
    cfg.ecumaster_base_id = 0x600U;
    cfg.can_timeout_ms = 500U;
    cfg.lambda_format = ValueFormatMode::Native;
    cfg.stoich_afr = 14.7f;

    cfg.parameter_visible.fill(true);

    cfg.tiles[0] = tile(ParameterId::VehicleSpeed, 0U);
    cfg.tiles[1] = tile(ParameterId::Map, 2U);
    cfg.tiles[2] = tile(ParameterId::Lambda, 2U);
    cfg.tiles[3] = tile(ParameterId::Clt, 0U);
    cfg.tiles[4] = tile(ParameterId::Iat, 0U);
    cfg.tiles[5] = tile(ParameterId::OilPressure, 1U);
    cfg.tiles[6] = tile(ParameterId::OilTemperature, 0U);
    cfg.tiles[7] = tile(ParameterId::FuelPressure, 1U);
    cfg.tiles[8] = tile(ParameterId::BatteryVoltage, 1U);
    cfg.tiles[9] = tile(ParameterId::Tps, 0U);
    cfg.tiles[10] = tile(ParameterId::BoostTarget, 2U);
    cfg.tiles[11] = tile(ParameterId::EthanolContent, 0U);

    cfg.track_tiles[0] = tile(ParameterId::VehicleSpeed, 0U);
    cfg.track_tiles[1] = tile(ParameterId::Map, 2U);
    cfg.track_tiles[2] = tile(ParameterId::Lambda, 2U);
    cfg.track_tiles[3] = tile(ParameterId::OilPressure, 1U);
    cfg.track_tiles[4] = tile(ParameterId::Clt, 0U);
    cfg.track_tiles[5] = tile(ParameterId::Iat, 0U, false);
    cfg.track_tiles[6] = tile(ParameterId::OilTemperature, 0U, false);
    cfg.track_tiles[7] = tile(ParameterId::FuelPressure, 1U, false);
    cfg.track_tiles[8] = tile(ParameterId::BatteryVoltage, 1U, false);
    cfg.track_tiles[9] = tile(ParameterId::Tps, 0U, false);
    cfg.track_tiles[10] = tile(ParameterId::BoostTarget, 2U, false);
    cfg.track_tiles[11] = tile(ParameterId::EthanolContent, 0U, false);

    for (auto& warning : cfg.warnings) {
        warning = WarningConfig{};
    }

    return cfg;
}

bool AppConfig::schemaCompatible(uint32_t version) {
    return version == kSchemaVersion;
}

bool AppConfig::validSchema() const {
    return schemaCompatible(schema_version);
}

void AppConfig::validate() {
    if (!supportedBitrate(can_bitrate)) {
        can_bitrate = 1000000U;
    }

    if (ecumaster_base_id > 0x7F8U) {
        ecumaster_base_id = 0x600U;
    }

    if (can_timeout_ms < 50U || can_timeout_ms > 10000U) {
        can_timeout_ms = 500U;
    }

    if (!std::isfinite(stoich_afr) || stoich_afr < 5.0f || stoich_afr > 20.0f) {
        stoich_afr = 14.7f;
    }

    const uint8_t format = static_cast<uint8_t>(lambda_format);
    if (format > static_cast<uint8_t>(ValueFormatMode::Afr)) {
        lambda_format = ValueFormatMode::Native;
    }

    const uint8_t source = static_cast<uint8_t>(data_source);
    if (source > static_cast<uint8_t>(DataSource::Rusefi)) {
        data_source = DataSource::Ecumaster;
    }

    validateTiles(tiles);
    validateTiles(track_tiles);
}

float AppConfig::lambdaToAfr(float lambda) const {
    return lambda * stoich_afr;
}
