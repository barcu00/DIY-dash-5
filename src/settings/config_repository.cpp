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

struct LegacyAppConfigV1 {
    uint32_t schema_version = 1U;
    DataSource data_source = DataSource::Demo;
    uint8_t brightness_percent = 100U;
    CanSettings can{};
    LegacyShiftLightConfigV1 shift{};
    UnitSettings units{};
    std::array<TileConfig, AppConfig::kDashTileCount> dash_tiles{};
    std::array<TileConfig, AppConfig::kTrackTileCount> track_tiles{};
};

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
    migrated.dash_tiles = legacy.dash_tiles;
    migrated.track_tiles = legacy.track_tiles;
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
    shift.red_rpm = std::min<uint16_t>(
        shift.red_rpm,
        static_cast<uint16_t>(shift.flash_rpm - kShiftRpmStep));
    shift.start_rpm = std::min<uint16_t>(
        shift.start_rpm,
        static_cast<uint16_t>(shift.red_rpm - kShiftRpmStep));
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
        (void)backend_.write(&config, sizeof(config));
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
