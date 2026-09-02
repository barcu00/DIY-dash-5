#pragma once

#include <array>
#include <cstdint>

#include "settings/app_config.h"

// Exact persisted OpenDash v0.2 layout used before schema v3 introduced
// three independent DASH profiles. Keep field order/types stable so an old
// Preferences blob can be identified by sizeof() and decoded safely.
struct LegacyAppConfigV2 {
    static constexpr uint32_t kSchemaVersion = 2U;

    uint32_t schema_version = kSchemaVersion;
    DataSource data_source = DataSource::Ecumaster;
    uint32_t can_bitrate = 1000000U;
    uint16_t ecumaster_base_id = 0x600U;
    uint32_t can_timeout_ms = 500U;

    ValueFormatMode lambda_format = ValueFormatMode::Native;
    float stoich_afr = 14.7f;

    std::array<TileConfig, AppConfig::kTileCount> tiles{};
    std::array<TileConfig, AppConfig::kTileCount> track_tiles{};
    std::array<bool, AppConfig::kParameterCount> parameter_visible{};
    std::array<WarningConfig, AppConfig::kParameterCount> warnings{};
};

class AppConfigMigration {
public:
    static bool fromV2(const LegacyAppConfigV2& legacy, AppConfig& out);
};
