#include <unity.h>

#include "settings/app_config.h"
#include "settings/app_config_migration.h"

namespace {
void test_v2_migration_preserves_backend_and_first_dash_page() {
    LegacyAppConfigV2 old_cfg{};
    old_cfg.schema_version = 2U;
    old_cfg.data_source = DataSource::Mock;
    old_cfg.can_bitrate = 500000U;
    old_cfg.ecumaster_base_id = 0x640U;
    old_cfg.can_timeout_ms = 900U;
    old_cfg.lambda_format = ValueFormatMode::Afr;
    old_cfg.stoich_afr = 14.1f;
    old_cfg.parameter_visible.fill(true);
    old_cfg.warnings.fill(WarningConfig{});

    for (uint8_t i = 0; i < AppConfig::kTileCount; ++i) {
        old_cfg.tiles[i].parameter = static_cast<ParameterId>(i);
        old_cfg.tiles[i].visible = i != 3U;
        old_cfg.track_tiles[i].parameter = static_cast<ParameterId>(20U + i);
    }
    old_cfg.warnings[7].mode = WarningMode::High;
    old_cfg.warnings[7].warning_threshold = 91.0f;
    old_cfg.parameter_visible[7] = false;

    AppConfig migrated{};
    TEST_ASSERT_TRUE(AppConfigMigration::fromV2(old_cfg, migrated));
    TEST_ASSERT_EQUAL_UINT32(3U, migrated.schema_version);
    TEST_ASSERT_EQUAL(static_cast<int>(DataSource::Mock), static_cast<int>(migrated.data_source));
    TEST_ASSERT_EQUAL_UINT32(500000U, migrated.can_bitrate);
    TEST_ASSERT_EQUAL_HEX16(0x640U, migrated.ecumaster_base_id);
    TEST_ASSERT_EQUAL_UINT32(900U, migrated.can_timeout_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.1f, migrated.stoich_afr);
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Afr), static_cast<int>(migrated.lambda_format));

    TEST_ASSERT_EQUAL(static_cast<int>(old_cfg.tiles[4].parameter),
                      static_cast<int>(migrated.dash_tiles[0][4].parameter));
    TEST_ASSERT_EQUAL(old_cfg.tiles[3].visible, migrated.dash_tiles[0][3].visible);
    TEST_ASSERT_EQUAL(static_cast<int>(old_cfg.track_tiles[2].parameter),
                      static_cast<int>(migrated.track_tiles[2].parameter));
    TEST_ASSERT_EQUAL(old_cfg.parameter_visible[7], migrated.parameter_visible[7]);
    TEST_ASSERT_EQUAL(static_cast<int>(old_cfg.warnings[7].mode),
                      static_cast<int>(migrated.warnings[7].mode));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 91.0f, migrated.warnings[7].warning_threshold);

    AppConfig defaults{};
    AppConfig::resetToDefaults(defaults);
    for (uint8_t slot = 0; slot < AppConfig::kTileCount; ++slot) {
        TEST_ASSERT_EQUAL(static_cast<int>(defaults.dash_tiles[1][slot].parameter),
                          static_cast<int>(migrated.dash_tiles[1][slot].parameter));
        TEST_ASSERT_EQUAL(static_cast<int>(defaults.dash_tiles[2][slot].parameter),
                          static_cast<int>(migrated.dash_tiles[2][slot].parameter));
    }
}

void test_migration_rejects_wrong_legacy_schema() {
    LegacyAppConfigV2 old_cfg{};
    old_cfg.schema_version = 1U;
    AppConfig migrated{};
    TEST_ASSERT_FALSE(AppConfigMigration::fromV2(old_cfg, migrated));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_v2_migration_preserves_backend_and_first_dash_page);
    RUN_TEST(test_migration_rejects_wrong_legacy_schema);
    return UNITY_END();
}
