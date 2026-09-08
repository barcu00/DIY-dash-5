#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <unity.h>

// Production changes caught: corrupt configuration being accepted, failed
// writes changing runtime state, or reset erasing without restoring defaults.

#include "settings/config_repository.h"

namespace {
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

template <std::size_t Count>
std::array<LegacyTileConfigV2, Count> legacyTiles(
    const std::array<TileConfig, Count>& current) {
    std::array<LegacyTileConfigV2, Count> legacy{};
    for (std::size_t i = 0; i < Count; ++i) {
        legacy[i].parameter = current[i].parameter;
        legacy[i].visible = current[i].visible;
        legacy[i].decimals = current[i].decimals;
        legacy[i].warning = current[i].warning;
    }
    return legacy;
}

LegacyAppConfigV2 legacyV2Defaults() {
    const AppConfig defaults = AppConfig::defaults();
    LegacyAppConfigV2 legacy;
    legacy.data_source = defaults.data_source;
    legacy.brightness_percent = defaults.brightness_percent;
    legacy.can = defaults.can;
    legacy.shift = defaults.shift;
    legacy.units = defaults.units;
    legacy.dash_tiles = legacyTiles(defaults.dash_tiles);
    legacy.track_tiles = legacyTiles(defaults.track_tiles);
    return legacy;
}

class MemoryBackend : public ConfigBackend {
public:
    std::size_t storedSize() const override {
        return has_value ? stored_size : 0U;
    }

    bool read(void* data, std::size_t size) override {
        if (!has_value || size != stored_size) {
            return false;
        }
        std::memcpy(data, bytes.data(), size);
        return true;
    }

    bool write(const void* data, std::size_t size) override {
        if (fail_writes || size > bytes.size()) {
            return false;
        }
        std::memcpy(bytes.data(), data, size);
        stored_size = size;
        has_value = true;
        return true;
    }

    bool erase() override {
        if (fail_erases) {
            return false;
        }
        has_value = false;
        stored_size = 0U;
        return true;
    }

    std::array<uint8_t, sizeof(AppConfig)> bytes{};
    std::size_t stored_size = 0U;
    bool has_value = false;
    bool fail_writes = false;
    bool fail_erases = false;
};
}  // namespace

void test_missing_configuration_loads_safe_defaults() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig loaded{};

    const LoadResult result = repository.load(loaded);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::DefaultsUsed),
                            static_cast<uint8_t>(result));
    TEST_ASSERT_EQUAL_UINT32(AppConfig::kSchemaVersion, loaded.schema_version);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(loaded.data_source));
}

void test_schema_mismatch_loads_safe_defaults() {
    MemoryBackend backend;
    AppConfig incompatible = AppConfig::defaults();
    incompatible.schema_version = 0U;
    TEST_ASSERT_TRUE(backend.write(&incompatible, sizeof(incompatible)));
    ConfigRepository repository(backend);
    AppConfig loaded{};

    const LoadResult result = repository.load(loaded);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::DefaultsUsed),
                            static_cast<uint8_t>(result));
    TEST_ASSERT_EQUAL_UINT32(AppConfig::kSchemaVersion, loaded.schema_version);
}

void test_valid_configuration_round_trips_through_backend() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.brightness_percent = 40U;
    candidate.dash_tiles[3].visible = false;

    TEST_ASSERT_TRUE(repository.saveCandidate(candidate, runtime));
    AppConfig reloaded{};
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::Loaded),
                            static_cast<uint8_t>(repository.load(reloaded)));
    TEST_ASSERT_EQUAL_UINT8(40U, reloaded.brightness_percent);
    TEST_ASSERT_FALSE(reloaded.dash_tiles[3].visible);
}

void test_schema_v2_shift_values_above_new_limit_are_normalized_in_place() {
    MemoryBackend backend;
    LegacyAppConfigV2 stored = legacyV2Defaults();
    stored.brightness_percent = 40U;
    stored.dash_tiles[3].visible = false;
    stored.shift = ShiftLightConfig{8500U, 9500U, 10500U, 12000U, true};
    TEST_ASSERT_TRUE(backend.write(&stored, sizeof(stored)));
    ConfigRepository repository(backend);
    AppConfig loaded{};

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::Migrated),
                            static_cast<uint8_t>(repository.load(loaded)));
    TEST_ASSERT_EQUAL_UINT16(8500U, loaded.shift.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(9500U, loaded.shift.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(10000U, loaded.shift.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(10000U, loaded.shift.max_rpm);
    TEST_ASSERT_EQUAL_UINT8(40U, loaded.brightness_percent);
    TEST_ASSERT_FALSE(loaded.dash_tiles[3].visible);
    TEST_ASSERT_TRUE(loaded.dash_tiles[3].temperature_bar.enabled);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 75.0f, loaded.dash_tiles[3].temperature_bar.ready_native);
}

void test_schema_v2_normalization_write_failure_is_reported() {
    MemoryBackend backend;
    LegacyAppConfigV2 stored = legacyV2Defaults();
    stored.shift = ShiftLightConfig{8500U, 9500U, 10500U, 12000U, true};
    TEST_ASSERT_TRUE(backend.write(&stored, sizeof(stored)));
    backend.fail_writes = true;
    ConfigRepository repository(backend);
    AppConfig loaded{};

    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(LoadResult::MigrationWriteFailed),
        static_cast<uint8_t>(repository.load(loaded)));
    TEST_ASSERT_EQUAL_UINT16(10000U, loaded.shift.max_rpm);
}

void test_schema_v2_custom_layout_gets_bar_defaults_for_its_parameters() {
    MemoryBackend backend;
    LegacyAppConfigV2 stored = legacyV2Defaults();
    stored.dash_tiles[0].parameter = ParameterId::Clt;
    stored.dash_tiles[3].parameter = ParameterId::Iat;
    TEST_ASSERT_TRUE(backend.write(&stored, sizeof(stored)));
    ConfigRepository repository(backend);
    AppConfig loaded{};

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::Migrated),
                            static_cast<uint8_t>(repository.load(loaded)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Clt),
                            static_cast<uint8_t>(loaded.dash_tiles[0].parameter));
    TEST_ASSERT_TRUE(loaded.dash_tiles[0].temperature_bar.enabled);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 75.0f, loaded.dash_tiles[0].temperature_bar.ready_native);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Iat),
                            static_cast<uint8_t>(loaded.dash_tiles[3].parameter));
    TEST_ASSERT_FALSE(loaded.dash_tiles[3].temperature_bar.enabled);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 40.0f, loaded.dash_tiles[3].temperature_bar.ready_native);
}

void test_schema_v1_is_migrated_without_losing_user_settings() {
    MemoryBackend backend;
    const AppConfig defaults = AppConfig::defaults();
    LegacyAppConfigV1 legacy;
    legacy.data_source = DataSource::Can;
    legacy.brightness_percent = 40U;
    legacy.can = defaults.can;
    legacy.shift = LegacyShiftLightConfigV1{5000U, 6500U, 7800U};
    legacy.units = defaults.units;
    legacy.dash_tiles = legacyTiles(defaults.dash_tiles);
    legacy.track_tiles = legacyTiles(defaults.track_tiles);
    legacy.dash_tiles[3].visible = false;
    legacy.track_tiles[2].decimals = 2U;
    TEST_ASSERT_TRUE(backend.write(&legacy, sizeof(legacy)));
    ConfigRepository repository(backend);
    AppConfig loaded{};

    const LoadResult result = repository.load(loaded);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::Migrated),
                            static_cast<uint8_t>(result));
    TEST_ASSERT_EQUAL_UINT32(AppConfig::kSchemaVersion, loaded.schema_version);
    TEST_ASSERT_EQUAL_UINT8(40U, loaded.brightness_percent);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Can),
                            static_cast<uint8_t>(loaded.data_source));
    TEST_ASSERT_EQUAL_UINT16(5000U, loaded.shift.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(6500U, loaded.shift.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(7800U, loaded.shift.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(7800U, loaded.shift.max_rpm);
    TEST_ASSERT_TRUE(loaded.shift.flash_enabled);
    TEST_ASSERT_FALSE(loaded.dash_tiles[3].visible);
    TEST_ASSERT_EQUAL_UINT8(2U, loaded.track_tiles[2].decimals);
    TEST_ASSERT_EQUAL_UINT32(sizeof(AppConfig), backend.stored_size);
}

void test_schema_v1_high_shift_values_are_normalized_without_losing_settings() {
    MemoryBackend backend;
    const AppConfig defaults = AppConfig::defaults();
    LegacyAppConfigV1 legacy;
    legacy.brightness_percent = 40U;
    legacy.can = defaults.can;
    legacy.shift = LegacyShiftLightConfigV1{9000U, 11000U, 13000U};
    legacy.units = defaults.units;
    legacy.dash_tiles = legacyTiles(defaults.dash_tiles);
    legacy.track_tiles = legacyTiles(defaults.track_tiles);
    legacy.dash_tiles[3].visible = false;
    TEST_ASSERT_TRUE(backend.write(&legacy, sizeof(legacy)));
    ConfigRepository repository(backend);
    AppConfig loaded{};

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LoadResult::Migrated),
                            static_cast<uint8_t>(repository.load(loaded)));
    TEST_ASSERT_EQUAL_UINT16(9000U, loaded.shift.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(9900U, loaded.shift.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(10000U, loaded.shift.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(10000U, loaded.shift.max_rpm);
    TEST_ASSERT_EQUAL_UINT8(40U, loaded.brightness_percent);
    TEST_ASSERT_FALSE(loaded.dash_tiles[3].visible);
}

void test_failed_v1_rewrite_reports_failure_but_keeps_migrated_runtime() {
    MemoryBackend backend;
    const AppConfig defaults = AppConfig::defaults();
    LegacyAppConfigV1 legacy;
    legacy.brightness_percent = 40U;
    legacy.can = defaults.can;
    legacy.shift = LegacyShiftLightConfigV1{5000U, 6500U, 7800U};
    legacy.units = defaults.units;
    legacy.dash_tiles = legacyTiles(defaults.dash_tiles);
    legacy.track_tiles = legacyTiles(defaults.track_tiles);
    TEST_ASSERT_TRUE(backend.write(&legacy, sizeof(legacy)));
    backend.fail_writes = true;
    ConfigRepository repository(backend);
    AppConfig loaded{};

    const LoadResult result = repository.load(loaded);

    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(LoadResult::MigrationWriteFailed),
        static_cast<uint8_t>(result));
    TEST_ASSERT_EQUAL_UINT8(40U, loaded.brightness_percent);
    TEST_ASSERT_EQUAL_UINT16(7800U, loaded.shift.flash_rpm);
    TEST_ASSERT_EQUAL_UINT32(sizeof(LegacyAppConfigV1), backend.stored_size);
}

void test_failed_save_keeps_previous_runtime_configuration() {
    MemoryBackend backend;
    backend.fail_writes = true;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.brightness_percent = 40U;

    TEST_ASSERT_FALSE(repository.saveCandidate(candidate, runtime));
    TEST_ASSERT_EQUAL_UINT8(100U, runtime.brightness_percent);
}

void test_invalid_candidate_is_not_written_or_applied() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    AppConfig candidate = runtime;
    candidate.shift =
        ShiftLightConfig{7000U, 6000U, 5000U, 5000U, true};

    TEST_ASSERT_FALSE(repository.saveCandidate(candidate, runtime));
    TEST_ASSERT_FALSE(backend.has_value);
    TEST_ASSERT_EQUAL_UINT16(5500U, runtime.shift.start_rpm);
}

void test_reset_erases_storage_and_restores_defaults_only_after_success() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    runtime.brightness_percent = 30U;

    TEST_ASSERT_TRUE(repository.reset(runtime));
    TEST_ASSERT_EQUAL_UINT8(100U, runtime.brightness_percent);
    TEST_ASSERT_FALSE(backend.has_value);

    runtime.brightness_percent = 30U;
    backend.fail_erases = true;
    TEST_ASSERT_FALSE(repository.reset(runtime));
    TEST_ASSERT_EQUAL_UINT8(30U, runtime.brightness_percent);
}

void test_reset_dash_layout_preserves_track_and_other_settings() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    runtime.brightness_percent = 40U;
    runtime.dash_tiles[0].visible = false;
    runtime.track_tiles[0].visible = false;

    TEST_ASSERT_TRUE(repository.resetLayout(PageId::Dash, runtime));

    TEST_ASSERT_TRUE(runtime.dash_tiles[0].visible);
    TEST_ASSERT_FALSE(runtime.track_tiles[0].visible);
    TEST_ASSERT_EQUAL_UINT8(40U, runtime.brightness_percent);
}

void test_reset_track_layout_preserves_dash_and_other_settings() {
    MemoryBackend backend;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    runtime.data_source = DataSource::Can;
    runtime.dash_tiles[0].visible = false;
    runtime.track_tiles[0].visible = false;

    TEST_ASSERT_TRUE(repository.resetLayout(PageId::Track, runtime));

    TEST_ASSERT_FALSE(runtime.dash_tiles[0].visible);
    TEST_ASSERT_TRUE(runtime.track_tiles[0].visible);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Can),
                            static_cast<uint8_t>(runtime.data_source));
}

void test_failed_layout_reset_keeps_runtime_configuration() {
    MemoryBackend backend;
    backend.fail_writes = true;
    ConfigRepository repository(backend);
    AppConfig runtime = AppConfig::defaults();
    runtime.dash_tiles[0].visible = false;

    TEST_ASSERT_FALSE(repository.resetLayout(PageId::Dash, runtime));

    TEST_ASSERT_FALSE(runtime.dash_tiles[0].visible);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_missing_configuration_loads_safe_defaults);
    RUN_TEST(test_schema_mismatch_loads_safe_defaults);
    RUN_TEST(test_valid_configuration_round_trips_through_backend);
    RUN_TEST(test_schema_v2_shift_values_above_new_limit_are_normalized_in_place);
    RUN_TEST(test_schema_v2_normalization_write_failure_is_reported);
    RUN_TEST(test_schema_v2_custom_layout_gets_bar_defaults_for_its_parameters);
    RUN_TEST(test_schema_v1_is_migrated_without_losing_user_settings);
    RUN_TEST(test_schema_v1_high_shift_values_are_normalized_without_losing_settings);
    RUN_TEST(test_failed_v1_rewrite_reports_failure_but_keeps_migrated_runtime);
    RUN_TEST(test_failed_save_keeps_previous_runtime_configuration);
    RUN_TEST(test_invalid_candidate_is_not_written_or_applied);
    RUN_TEST(test_reset_erases_storage_and_restores_defaults_only_after_success);
    RUN_TEST(test_reset_dash_layout_preserves_track_and_other_settings);
    RUN_TEST(test_reset_track_layout_preserves_dash_and_other_settings);
    RUN_TEST(test_failed_layout_reset_keeps_runtime_configuration);
    return UNITY_END();
}
