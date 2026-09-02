#include "app_config.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr bool supportedBitrate(uint32_t bitrate) {
    return bitrate == 125000U || bitrate == 250000U ||
           bitrate == 500000U || bitrate == 1000000U;
}

TileConfig tile(ParameterId parameter, uint8_t decimals, bool visible = true,
                ValueFormatMode format = ValueFormatMode::Native) {
    TileConfig cfg{};
    cfg.parameter = parameter;
    cfg.visible = visible;
    cfg.decimals = decimals;
    cfg.value_format = format;
    return cfg;
}

void validateTiles(AppConfig::TileProfile& tiles) {
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

void setDash1(AppConfig::TileProfile& p) {
    p[0] = tile(ParameterId::Rpm, 0U);
    p[1] = tile(ParameterId::VehicleSpeed, 0U);
    p[2] = tile(ParameterId::BoostTarget, 2U);
    p[3] = tile(ParameterId::Map, 2U);
    p[4] = tile(ParameterId::Iat, 0U);
    p[5] = tile(ParameterId::Clt, 0U);
    p[6] = tile(ParameterId::OilTemperature, 0U);
    p[7] = tile(ParameterId::OilPressure, 1U);
    p[8] = tile(ParameterId::Lambda, 2U, true, ValueFormatMode::Afr);
    p[9] = tile(ParameterId::Lambda, 2U);
    p[10] = tile(ParameterId::IgnitionAngle, 1U);
    p[11] = tile(ParameterId::Tps, 0U);
}

void setDash2(AppConfig::TileProfile& p) {
    p[0] = tile(ParameterId::FuelPressure, 1U);
    p[1] = tile(ParameterId::BatteryVoltage, 1U);
    p[2] = tile(ParameterId::InjectorPulseWidth, 1U);
    p[3] = tile(ParameterId::Dwell, 1U);
    p[4] = tile(ParameterId::Knocking, 0U);
    p[5] = tile(ParameterId::Egt1, 0U);
    p[6] = tile(ParameterId::Gear, 0U);
    p[7] = tile(ParameterId::DbwPosition, 0U);
    p[8] = tile(ParameterId::DbwTarget, 0U);
    p[9] = tile(ParameterId::TcTorqueReduction, 0U);
    p[10] = tile(ParameterId::EthanolContent, 0U);
    p[11] = tile(ParameterId::FuelUsed, 1U);
}

void setDash3(AppConfig::TileProfile& p) {
    p[0] = tile(ParameterId::BarometricPressure, 0U);
    p[1] = tile(ParameterId::EcuTemperature, 0U);
    p[2] = tile(ParameterId::Egt2, 0U);
    p[3] = tile(ParameterId::LambdaTarget, 2U);
    p[4] = tile(ParameterId::LambdaCorrection, 1U);
    p[5] = tile(ParameterId::Ain1, 2U);
    p[6] = tile(ParameterId::Ain2, 2U);
    p[7] = tile(ParameterId::Pwm1, 0U);
    p[8] = tile(ParameterId::Pwm2, 0U);
    p[9] = tile(ParameterId::TractionControlActive, 0U);
    p[10] = tile(ParameterId::LaunchControlActive, 0U);
    p[11] = tile(ParameterId::EcuError, 0U);
}
}

void AppConfig::resetToDefaults(AppConfig& cfg) {
    cfg.schema_version = kSchemaVersion;
    cfg.data_source = DataSource::Ecumaster;
    cfg.can_bitrate = 1000000U;
    cfg.ecumaster_base_id = 0x600U;
    cfg.can_timeout_ms = 500U;
    cfg.lambda_format = ValueFormatMode::Native;
    cfg.stoich_afr = 14.7f;

    for (auto& profile : cfg.dash_tiles) profile.fill(TileConfig{});
    cfg.track_tiles.fill(TileConfig{});
    cfg.parameter_visible.fill(true);
    cfg.warnings.fill(WarningConfig{});

    setDash1(cfg.dash_tiles[0]);
    setDash2(cfg.dash_tiles[1]);
    setDash3(cfg.dash_tiles[2]);

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
}

AppConfig AppConfig::defaults() {
    AppConfig cfg{};
    resetToDefaults(cfg);
    return cfg;
}

bool AppConfig::schemaCompatible(uint32_t version) {
    return version == kSchemaVersion;
}

bool AppConfig::validSchema() const {
    return schemaCompatible(schema_version);
}

void AppConfig::validate() {
    if (!supportedBitrate(can_bitrate)) can_bitrate = 1000000U;
    if (ecumaster_base_id > 0x7F8U) ecumaster_base_id = 0x600U;
    if (can_timeout_ms < 50U || can_timeout_ms > 10000U) can_timeout_ms = 500U;
    if (!std::isfinite(stoich_afr) || stoich_afr < 5.0f || stoich_afr > 20.0f) stoich_afr = 14.7f;

    const uint8_t format = static_cast<uint8_t>(lambda_format);
    if (format > static_cast<uint8_t>(ValueFormatMode::Afr)) lambda_format = ValueFormatMode::Native;

    const uint8_t source = static_cast<uint8_t>(data_source);
    if (source > static_cast<uint8_t>(DataSource::Rusefi)) data_source = DataSource::Ecumaster;

    for (auto& profile : dash_tiles) validateTiles(profile);
    validateTiles(track_tiles);
}

float AppConfig::lambdaToAfr(float lambda) const {
    return lambda * stoich_afr;
}
