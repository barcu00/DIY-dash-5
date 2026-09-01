#include <unity.h>

#include "settings/app_config.h"

namespace {
void test_default_can_configuration_targets_ecumaster() {
    const AppConfig cfg = AppConfig::defaults();
    TEST_ASSERT_EQUAL(static_cast<int>(DataSource::Ecumaster), static_cast<int>(cfg.data_source));
    TEST_ASSERT_EQUAL_UINT32(1000000U, cfg.can_bitrate);
    TEST_ASSERT_EQUAL_HEX16(0x600U, cfg.ecumaster_base_id);
    TEST_ASSERT_EQUAL_UINT32(500U, cfg.can_timeout_ms);
}

void test_default_lambda_format_and_stoich() {
    const AppConfig cfg = AppConfig::defaults();
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Native), static_cast<int>(cfg.lambda_format));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, cfg.stoich_afr);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.495f, cfg.lambdaToAfr(0.85f));
}

void test_default_tiles_are_visible_and_have_expected_core_mappings() {
    const AppConfig cfg = AppConfig::defaults();
    TEST_ASSERT_TRUE(cfg.tiles[0].visible);
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::VehicleSpeed), static_cast<int>(cfg.tiles[0].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Map), static_cast<int>(cfg.tiles[1].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Lambda), static_cast<int>(cfg.tiles[2].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Clt), static_cast<int>(cfg.tiles[3].parameter));
}

void test_tile_and_parameter_visibility_are_configurable_independently() {
    AppConfig cfg = AppConfig::defaults();
    cfg.tiles[2].visible = false;
    cfg.parameter_visible[parameterIndex(ParameterId::Lambda)] = false;

    TEST_ASSERT_FALSE(cfg.tiles[2].visible);
    TEST_ASSERT_FALSE(cfg.parameter_visible[parameterIndex(ParameterId::Lambda)]);
    TEST_ASSERT_TRUE(cfg.parameter_visible[parameterIndex(ParameterId::OilPressure)]);
}

void test_afr_mode_uses_configurable_stoich() {
    AppConfig cfg = AppConfig::defaults();
    cfg.lambda_format = ValueFormatMode::Afr;
    cfg.stoich_afr = 9.765f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.812f, cfg.lambdaToAfr(0.8f));
}

void test_validation_repairs_invalid_can_and_afr_values() {
    AppConfig cfg = AppConfig::defaults();
    cfg.can_bitrate = 333333U;
    cfg.ecumaster_base_id = 0x900U;
    cfg.can_timeout_ms = 0U;
    cfg.stoich_afr = -1.0f;

    cfg.validate();
    TEST_ASSERT_EQUAL_UINT32(1000000U, cfg.can_bitrate);
    TEST_ASSERT_EQUAL_HEX16(0x600U, cfg.ecumaster_base_id);
    TEST_ASSERT_EQUAL_UINT32(500U, cfg.can_timeout_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, cfg.stoich_afr);
}

void test_schema_mismatch_requires_factory_defaults() {
    AppConfig cfg = AppConfig::defaults();
    TEST_ASSERT_TRUE(AppConfig::schemaCompatible(AppConfig::kSchemaVersion));
    TEST_ASSERT_FALSE(AppConfig::schemaCompatible(AppConfig::kSchemaVersion + 1U));

    cfg.schema_version = AppConfig::kSchemaVersion + 1U;
    TEST_ASSERT_FALSE(cfg.validSchema());
}

void test_tile_config_supports_hide_label_icon_and_decimals() {
    AppConfig cfg = AppConfig::defaults();
    TileConfig& tile = cfg.tiles[0];
    tile.visible = false;
    tile.custom_label_enabled = true;
    tile.custom_label[0] = 'V';
    tile.custom_label[1] = 0;
    tile.icon_enabled = false;
    tile.icon = 42U;
    tile.decimals = 2U;

    TEST_ASSERT_FALSE(tile.visible);
    TEST_ASSERT_TRUE(tile.custom_label_enabled);
    TEST_ASSERT_EQUAL_STRING("V", tile.custom_label.data());
    TEST_ASSERT_FALSE(tile.icon_enabled);
    TEST_ASSERT_EQUAL_UINT16(42U, tile.icon);
    TEST_ASSERT_EQUAL_UINT8(2U, tile.decimals);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_default_can_configuration_targets_ecumaster);
    RUN_TEST(test_default_lambda_format_and_stoich);
    RUN_TEST(test_default_tiles_are_visible_and_have_expected_core_mappings);
    RUN_TEST(test_tile_and_parameter_visibility_are_configurable_independently);
    RUN_TEST(test_afr_mode_uses_configurable_stoich);
    RUN_TEST(test_validation_repairs_invalid_can_and_afr_values);
    RUN_TEST(test_schema_mismatch_requires_factory_defaults);
    RUN_TEST(test_tile_config_supports_hide_label_icon_and_decimals);
    return UNITY_END();
}
