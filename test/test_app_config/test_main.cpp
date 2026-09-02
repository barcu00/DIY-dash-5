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

void test_three_dashboard_pages_have_render_priority_defaults() {
    const AppConfig cfg = AppConfig::defaults();
    TEST_ASSERT_EQUAL_UINT8(3U, AppConfig::kDashPageCount);
    TEST_ASSERT_EQUAL_UINT8(12U, AppConfig::kTileCount);
    TEST_ASSERT_EQUAL_UINT32(3U, AppConfig::kSchemaVersion);

    const auto& dash1 = cfg.dash_tiles[0];
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Rpm), static_cast<int>(dash1[0].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::VehicleSpeed), static_cast<int>(dash1[1].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::BoostTarget), static_cast<int>(dash1[2].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Map), static_cast<int>(dash1[3].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Iat), static_cast<int>(dash1[4].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Clt), static_cast<int>(dash1[5].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::OilTemperature), static_cast<int>(dash1[6].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::OilPressure), static_cast<int>(dash1[7].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Lambda), static_cast<int>(dash1[8].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Afr), static_cast<int>(dash1[8].value_format));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Lambda), static_cast<int>(dash1[9].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Native), static_cast<int>(dash1[9].value_format));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::IgnitionAngle), static_cast<int>(dash1[10].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Tps), static_cast<int>(dash1[11].parameter));

    for (uint8_t page = 0; page < AppConfig::kDashPageCount; ++page) {
        for (const TileConfig& tile : cfg.dash_tiles[page]) {
            TEST_ASSERT_TRUE(parameterIndex(tile.parameter) < AppConfig::kParameterCount);
        }
    }
}

void test_three_dashboard_pages_are_configurable_independently() {
    AppConfig cfg = AppConfig::defaults();
    cfg.dash_tiles[0][0].parameter = ParameterId::BatteryVoltage;
    cfg.dash_tiles[1][0].parameter = ParameterId::FuelPressure;
    cfg.dash_tiles[2][0].parameter = ParameterId::BarometricPressure;

    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::BatteryVoltage), static_cast<int>(cfg.dash_tiles[0][0].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::FuelPressure), static_cast<int>(cfg.dash_tiles[1][0].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::BarometricPressure), static_cast<int>(cfg.dash_tiles[2][0].parameter));
}

void test_track_layout_has_independent_defaults() {
    const AppConfig cfg = AppConfig::defaults();
    TEST_ASSERT_TRUE(cfg.track_tiles[0].visible);
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::VehicleSpeed), static_cast<int>(cfg.track_tiles[0].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Map), static_cast<int>(cfg.track_tiles[1].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Lambda), static_cast<int>(cfg.track_tiles[2].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::OilPressure), static_cast<int>(cfg.track_tiles[3].parameter));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Clt), static_cast<int>(cfg.track_tiles[4].parameter));
    TEST_ASSERT_FALSE(cfg.track_tiles[5].visible);
}

void test_tile_and_parameter_visibility_are_configurable_independently() {
    AppConfig cfg = AppConfig::defaults();
    cfg.dash_tiles[0][8].visible = false;
    cfg.parameter_visible[parameterIndex(ParameterId::Lambda)] = false;

    TEST_ASSERT_FALSE(cfg.dash_tiles[0][8].visible);
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
    cfg.dash_tiles[2][7].parameter = static_cast<ParameterId>(0xFFFFU);

    cfg.validate();
    TEST_ASSERT_EQUAL_UINT32(1000000U, cfg.can_bitrate);
    TEST_ASSERT_EQUAL_HEX16(0x600U, cfg.ecumaster_base_id);
    TEST_ASSERT_EQUAL_UINT32(500U, cfg.can_timeout_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, cfg.stoich_afr);
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Rpm), static_cast<int>(cfg.dash_tiles[2][7].parameter));
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
    TileConfig& tile = cfg.dash_tiles[0][0];
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

void test_in_place_reset_restores_factory_configuration() {
    AppConfig cfg = AppConfig::defaults();
    cfg.data_source = DataSource::Mock;
    cfg.can_bitrate = 125000U;
    cfg.stoich_afr = 9.0f;
    cfg.dash_tiles[0][0].parameter = ParameterId::BatteryVoltage;
    cfg.dash_tiles[1][0].parameter = ParameterId::EthanolContent;
    cfg.track_tiles[5].visible = true;
    cfg.parameter_visible[parameterIndex(ParameterId::Lambda)] = false;
    cfg.warnings[parameterIndex(ParameterId::OilPressure)].mode = WarningMode::RpmCurve;
    cfg.warnings[parameterIndex(ParameterId::OilPressure)].rpm_curve_count = 3U;

    AppConfig::resetToDefaults(cfg);

    TEST_ASSERT_EQUAL_UINT32(3U, cfg.schema_version);
    TEST_ASSERT_EQUAL(static_cast<int>(DataSource::Ecumaster), static_cast<int>(cfg.data_source));
    TEST_ASSERT_EQUAL_UINT32(1000000U, cfg.can_bitrate);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, cfg.stoich_afr);
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Rpm), static_cast<int>(cfg.dash_tiles[0][0].parameter));
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(ParameterId::EthanolContent), static_cast<int>(cfg.dash_tiles[1][0].parameter));
    TEST_ASSERT_FALSE(cfg.track_tiles[5].visible);
    TEST_ASSERT_TRUE(cfg.parameter_visible[parameterIndex(ParameterId::Lambda)]);
    TEST_ASSERT_EQUAL(static_cast<int>(WarningMode::Off),
                      static_cast<int>(cfg.warnings[parameterIndex(ParameterId::OilPressure)].mode));
    TEST_ASSERT_EQUAL_UINT8(0U, cfg.warnings[parameterIndex(ParameterId::OilPressure)].rpm_curve_count);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_default_can_configuration_targets_ecumaster);
    RUN_TEST(test_default_lambda_format_and_stoich);
    RUN_TEST(test_three_dashboard_pages_have_render_priority_defaults);
    RUN_TEST(test_three_dashboard_pages_are_configurable_independently);
    RUN_TEST(test_track_layout_has_independent_defaults);
    RUN_TEST(test_tile_and_parameter_visibility_are_configurable_independently);
    RUN_TEST(test_afr_mode_uses_configurable_stoich);
    RUN_TEST(test_validation_repairs_invalid_can_and_afr_values);
    RUN_TEST(test_schema_mismatch_requires_factory_defaults);
    RUN_TEST(test_tile_config_supports_hide_label_icon_and_decimals);
    RUN_TEST(test_in_place_reset_restores_factory_configuration);
    return UNITY_END();
}
