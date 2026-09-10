#include "config_repository.h"

#include <algorithm>

namespace {
constexpr uint16_t kCurrentMaximumShiftRpm = 10000U;
constexpr uint16_t kShiftRpmStep = 100U;

struct LegacyShiftLightConfigV1 {
    uint16_t start_rpm = 5500U;
    uint16_t red_rpm = 7000U;
    uint16_t max_rpm = 8000U;
};

struct LegacyTileConfigV2 {
    ParameterId parameter = ParameterId::Rpm;
    bool visible = true;
    uint8_t decimals = 0U;
    TileWarningConfig warning{};
};

struct LegacyAppConfigV1 {
    uint32_t schema_version = 1U;
    DataSource data_source = DataSource::Demo;
    uint8_t brightness_percent = 100U;
    CanSettings can{};
    LegacyShiftLightConfigV1 shift{};
    UnitSettings units{};
    std::array<LegacyTileConfigV2, AppConfig::kDashTileCount> dash_tiles{};
    std::array<LegacyTileConfigV2, AppConfig::kTrackTileCount> track_tiles{};
};

struct LegacyAppConfigV2 {
    uint32_t schema_version = 2U;
    DataSource data_source = DataSource::Demo;
    uint8_t brightness_percent = 100U;
    CanSettings can{};
    ShiftLightConfig shift{};
    UnitSettings units{};
    std::array<LegacyTileConfigV2, AppConfig::kDashTileCount> dash_tiles{};
    std::array<LegacyTileConfigV2, AppConfig::kTrackTileCount> track_tiles{};
};

struct LegacyTemperatureBarConfigV3 {
    bool enabled = false;
    float minimum_native = 40.0f;
    float ready_native = 75.0f;
    float maximum_native = 130.0f;
};

struct LegacyTileConfigV3 {
    ParameterId parameter = ParameterId::Rpm;
    bool visible = true;
    uint8_t decimals = 0U;
    TileWarningConfig warning{};
    LegacyTemperatureBarConfigV3 temperature_bar{};
};

struct LegacyAppConfigV3 {
    uint32_t schema_version = 3U;
    DataSource data_source = DataSource::Demo;
    uint8_t brightness_percent = 100U;
    CanSettings can{};
    ShiftLightConfig shift{};
    UnitSettings units{};
    std::array<LegacyTileConfigV3, AppConfig::kDashTileCount> dash_tiles{};
    std::array<LegacyTileConfigV3, AppConfig::kTrackTileCount> track_tiles{};
};

struct LegacyTileConfigV4 {
    ParameterId parameter = ParameterId::Rpm;
    bool visible = true;
    uint8_t decimals = 0U;
    TileWarningConfig warning{};
    LegacyTemperatureBarConfigV3 temperature_bar{};
    FlagActiveColor flag_active_color = FlagActiveColor::Yellow;
};

struct LegacyAppConfigV4 {
    uint32_t schema_version = 4U;
    DataSource data_source = DataSource::Demo;
    uint8_t brightness_percent = 100U;
    CanSettings can{};
    ShiftLightConfig shift{};
    UnitSettings units{};
    std::array<LegacyTileConfigV4, AppConfig::kDashTileCount> dash_tiles{};
    std::array<LegacyTileConfigV4, AppConfig::kTrackTileCount> track_tiles{};
};

template <std::size_t Count>
void migrateTiles(const std::array<LegacyTileConfigV2, Count>& legacy,
                  std::array<TileConfig, Count>& migrated) {
    for (std::size_t i = 0U; i < Count; ++i) {
        migrated[i].parameter = legacy[i].parameter;
        migrated[i].visible = legacy[i].visible;
        migrated[i].decimals = legacy[i].decimals;
        migrated[i].warning = legacy[i].warning;
        migrated[i].temperature_bar =
            defaultTemperatureBarConfig(legacy[i].parameter);
    }
}

template <std::size_t Count>
void migrateTiles(const std::array<LegacyTileConfigV3, Count>& legacy,
                  std::array<TileConfig, Count>& migrated) {
    for (std::size_t i = 0U; i < Count; ++i) {
        migrated[i].parameter = legacy[i].parameter;
        migrated[i].visible = legacy[i].visible;
        migrated[i].decimals = legacy[i].decimals;
        migrated[i].warning = legacy[i].warning;
        migrated[i].temperature_bar = {
            legacy[i].temperature_bar.enabled,
            legacy[i].temperature_bar.minimum_native,
            legacy[i].temperature_bar.ready_native,
            legacy[i].temperature_bar.maximum_native,
            legacy[i].temperature_bar.maximum_native};
        migrated[i].flag_active_color = FlagActiveColor::Yellow;
    }
}

template <std::size_t Count>
void migrateTiles(const std::array<LegacyTileConfigV4, Count>& legacy,
                  std::array<TileConfig, Count>& migrated) {
    for (std::size_t i = 0U; i < Count; ++i) {
        migrated[i].parameter = legacy[i].parameter;
        migrated[i].visible = legacy[i].visible;
        migrated[i].decimals = legacy[i].decimals;
        migrated[i].warning = legacy[i].warning;
        migrated[i].temperature_bar = {
            legacy[i].temperature_bar.enabled,
            legacy[i].temperature_bar.minimum_native,
            legacy[i].temperature_bar.ready_native,
            legacy[i].temperature_bar.maximum_native,
            legacy[i].temperature_bar.maximum_native};
        migrated[i].flag_active_color = legacy[i].flag_active_color;
    }
}

bool normalizeLegacyShiftRange(ShiftLightConfig& shift);

LoadResult migrateV1(ConfigBackend& backend, AppConfig& config) {
    LegacyAppConfigV1 legacy{};
    if (!backend.read(&legacy, sizeof(legacy)) || legacy.schema_version != 1U) {
        return LoadResult::DefaultsUsed;
    }

    AppConfig migrated = AppConfig::defaults();
    migrated.data_source = legacy.data_source;
    migrated.brightness_percent = legacy.brightness_percent;
    migrated.can = legacy.can;
    migrated.shift.start_rpm = legacy.shift.start_rpm;
    migrated.shift.red_rpm = legacy.shift.red_rpm;
    migrated.shift.flash_rpm = legacy.shift.max_rpm;
    migrated.shift.max_rpm = legacy.shift.max_rpm;
    migrated.shift.flash_enabled = true;
    migrated.units = legacy.units;
    migrateTiles(legacy.dash_tiles, migrated.dash_tiles);
    migrateTiles(legacy.track_tiles, migrated.track_tiles);
    normalizeLegacyShiftRange(migrated.shift);
    if (!migrated.validate().valid) {
        return LoadResult::DefaultsUsed;
    }

    config = migrated;
    return backend.write(&config, sizeof(config))
               ? LoadResult::Migrated
               : LoadResult::MigrationWriteFailed;
}

LoadResult migrateV2(ConfigBackend& backend, AppConfig& config) {
    LegacyAppConfigV2 legacy{};
    if (!backend.read(&legacy, sizeof(legacy)) || legacy.schema_version != 2U) {
        return LoadResult::DefaultsUsed;
    }

    AppConfig migrated = AppConfig::defaults();
    migrated.data_source = legacy.data_source;
    migrated.brightness_percent = legacy.brightness_percent;
    migrated.can = legacy.can;
    migrated.shift = legacy.shift;
    migrated.units = legacy.units;
    migrateTiles(legacy.dash_tiles, migrated.dash_tiles);
    migrateTiles(legacy.track_tiles, migrated.track_tiles);
    normalizeLegacyShiftRange(migrated.shift);
    if (!migrated.validate().valid) {
        return LoadResult::DefaultsUsed;
    }

    config = migrated;
    return backend.write(&config, sizeof(config))
               ? LoadResult::Migrated
               : LoadResult::MigrationWriteFailed;
}

LoadResult migrateV3(ConfigBackend& backend, AppConfig& config) {
    LegacyAppConfigV3 legacy{};
    if (!backend.read(&legacy, sizeof(legacy)) || legacy.schema_version != 3U) {
        return LoadResult::DefaultsUsed;
    }

    AppConfig migrated = AppConfig::defaults();
    migrated.data_source = legacy.data_source;
    migrated.brightness_percent = legacy.brightness_percent;
    migrated.can = legacy.can;
    migrated.shift = legacy.shift;
    migrated.units = legacy.units;
    migrateTiles(legacy.dash_tiles, migrated.dash_tiles);
    migrateTiles(legacy.track_tiles, migrated.track_tiles);
    normalizeLegacyShiftRange(migrated.shift);
    if (!migrated.validate().valid) {
        return LoadResult::DefaultsUsed;
    }

    config = migrated;
    return backend.write(&config, sizeof(config))
               ? LoadResult::Migrated
               : LoadResult::MigrationWriteFailed;
}

LoadResult migrateV4(ConfigBackend& backend, AppConfig& config) {
    LegacyAppConfigV4 legacy{};
    if (!backend.read(&legacy, sizeof(legacy)) || legacy.schema_version != 4U) {
        return LoadResult::DefaultsUsed;
    }

    AppConfig migrated = AppConfig::defaults();
    migrated.data_source = legacy.data_source;
    migrated.brightness_percent = legacy.brightness_percent;
    migrated.can = legacy.can;
    migrated.shift = legacy.shift;
    migrated.units = legacy.units;
    migrateTiles(legacy.dash_tiles, migrated.dash_tiles);
    migrateTiles(legacy.track_tiles, migrated.track_tiles);
    normalizeLegacyShiftRange(migrated.shift);
    if (!migrated.validate().valid) {
        return LoadResult::DefaultsUsed;
    }

    config = migrated;
    return backend.write(&config, sizeof(config))
               ? LoadResult::Migrated
               : LoadResult::MigrationWriteFailed;
}

bool normalizeLegacyShiftRange(ShiftLightConfig& shift) {
    if (shift.start_rpm <= kCurrentMaximumShiftRpm &&
        shift.red_rpm <= kCurrentMaximumShiftRpm &&
        shift.flash_rpm <= kCurrentMaximumShiftRpm &&
        shift.max_rpm <= kCurrentMaximumShiftRpm) {
        return false;
    }

    shift.max_rpm = std::min<uint16_t>(shift.max_rpm,
                                        kCurrentMaximumShiftRpm);
    shift.flash_rpm = std::min<uint16_t>(shift.flash_rpm, shift.max_rpm);
    const uint16_t red_limit = shift.flash_rpm >= kShiftRpmStep
        ? static_cast<uint16_t>(shift.flash_rpm - kShiftRpmStep)
        : 0U;
    shift.red_rpm = std::min<uint16_t>(
        shift.red_rpm, red_limit);
    const uint16_t start_limit = shift.red_rpm >= kShiftRpmStep
        ? static_cast<uint16_t>(shift.red_rpm - kShiftRpmStep)
        : 0U;
    shift.start_rpm = std::min<uint16_t>(
        shift.start_rpm, start_limit);
    return true;
}
}  // namespace

ConfigRepository::ConfigRepository(ConfigBackend& backend) : backend_(backend) {}

LoadResult ConfigRepository::load(AppConfig& config) {
    const std::size_t stored_size = backend_.storedSize();
    if (stored_size == sizeof(LegacyAppConfigV1)) {
        const LoadResult migration = migrateV1(backend_, config);
        if (migration != LoadResult::DefaultsUsed) {
            return migration;
        }
    }
    if (stored_size == sizeof(LegacyAppConfigV2)) {
        const LoadResult migration = migrateV2(backend_, config);
        if (migration != LoadResult::DefaultsUsed) {
            return migration;
        }
    }
    if (stored_size == sizeof(LegacyAppConfigV3)) {
        const LoadResult migration = migrateV3(backend_, config);
        if (migration != LoadResult::DefaultsUsed) {
            return migration;
        }
    }
    if (stored_size == sizeof(LegacyAppConfigV4)) {
        const LoadResult migration = migrateV4(backend_, config);
        if (migration != LoadResult::DefaultsUsed) {
            return migration;
        }
    }

    AppConfig candidate{};
    if (stored_size != sizeof(candidate) ||
        !backend_.read(&candidate, sizeof(candidate)) ||
        candidate.schema_version != AppConfig::kSchemaVersion) {
        config = AppConfig::defaults();
        return LoadResult::DefaultsUsed;
    }

    const bool normalized = normalizeLegacyShiftRange(candidate.shift);
    if (!candidate.validate().valid) {
        config = AppConfig::defaults();
        return LoadResult::DefaultsUsed;
    }

    config = candidate;
    if (normalized) {
        return backend_.write(&config, sizeof(config))
            ? LoadResult::Loaded
            : LoadResult::MigrationWriteFailed;
    }
    return LoadResult::Loaded;
}

bool ConfigRepository::saveCandidate(const AppConfig& candidate,
                                     AppConfig& runtime_config) {
    AppConfig validated = candidate;
    if (validated.schema_version != AppConfig::kSchemaVersion ||
        !validated.validate().valid ||
        !backend_.write(&validated, sizeof(validated))) {
        return false;
    }

    runtime_config = validated;
    return true;
}

bool ConfigRepository::resetLayout(PageId page, AppConfig& runtime_config) {
    AppConfig candidate = runtime_config;
    const AppConfig defaults = AppConfig::defaults();
    if (page == PageId::Dash) {
        candidate.dash_tiles = defaults.dash_tiles;
    } else if (page == PageId::Track) {
        candidate.track_tiles = defaults.track_tiles;
    } else {
        return false;
    }
    return saveCandidate(candidate, runtime_config);
}

bool ConfigRepository::reset(AppConfig& runtime_config) {
    if (!backend_.erase()) {
        return false;
    }
    runtime_config = AppConfig::defaults();
    return true;
}
