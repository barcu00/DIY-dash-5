#include <cstring>

#include <unity.h>

// Production changes caught: wrong approved tile defaults, unsafe persisted
// values reaching runtime, or invalid shift-light ordering being accepted.

#include "settings/app_config.h"

void test_defaults_define_approved_dash_and_track_slots() {
    const AppConfig config = AppConfig::defaults();

    TEST_ASSERT_EQUAL_UINT32(AppConfig::kSchemaVersion, config.schema_version);
    TEST_ASSERT_EQUAL_UINT32(14U, config.dash_tiles.size());
    TEST_ASSERT_EQUAL_UINT32(12U, config.track_tiles.size());
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(ParameterId::Rpm),
        static_cast<uint8_t>(config.dash_tiles[4].parameter));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(ParameterId::Gear),
        static_cast<uint8_t>(config.dash_tiles[5].parameter));

    for (const TileConfig& tile : config.dash_tiles) {
        TEST_ASSERT_TRUE(tile.visible);
        TEST_ASSERT_TRUE(static_cast<std::size_t>(tile.parameter) <
                         parameterCount());
    }
    for (const TileConfig& tile : config.track_tiles) {
        TEST_ASSERT_TRUE(tile.visible);
        TEST_ASSERT_TRUE(static_cast<std::size_t>(tile.parameter) <
                         parameterCount());
    }
}

void test_defaults_use_demo_metric_and_shared_shift_configuration() {
    const AppConfig config = AppConfig::defaults();

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(config.data_source));
    TEST_ASSERT_EQUAL_UINT8(100U, config.brightness_percent);
    TEST_ASSERT_EQUAL_UINT32(500000U, config.can.bitrate);
    TEST_ASSERT_EQUAL_UINT32(500U, config.can.timeout_ms);
    TEST_ASSERT_EQUAL_STRING("none", config.can.profile_id.data());
    TEST_ASSERT_EQUAL_UINT16(5500U, config.shift.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(7000U, config.shift.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(7500U, config.shift.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(8000U, config.shift.max_rpm);
    TEST_ASSERT_TRUE(config.shift.flash_enabled);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TemperatureUnit::Celsius),
                            static_cast<uint8_t>(config.units.temperature));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PressureUnit::Bar),
                            static_cast<uint8_t>(config.units.pressure));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SpeedUnit::Kph),
                            static_cast<uint8_t>(config.units.speed));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(MixtureUnit::Lambda),
                            static_cast<uint8_t>(config.units.mixture));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, config.units.stoich_afr);
}

void test_validation_normalizes_unsafe_persisted_values() {
    AppConfig config = AppConfig::defaults();
    config.data_source = DataSource::None;
    config.brightness_percent = 0U;
    config.can.bitrate = 123456U;
    config.can.timeout_ms = 50U;
    config.units.stoich_afr = 0.0f;
    config.dash_tiles[0].parameter = static_cast<ParameterId>(255U);
    config.dash_tiles[0].decimals = 9U;
    config.dash_tiles[0].warning.hysteresis_native = -1.0f;
    config.dash_tiles[0].warning.delay_ms = 60000U;
    std::memset(config.can.profile_id.data(), 'x', config.can.profile_id.size());

    const ValidationResult result = config.validate();

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_TRUE(result.shift_order_valid);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(config.data_source));
    TEST_ASSERT_EQUAL_UINT8(20U, config.brightness_percent);
    TEST_ASSERT_EQUAL_UINT32(500000U, config.can.bitrate);
    TEST_ASSERT_EQUAL_UINT32(100U, config.can.timeout_ms);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, config.units.stoich_afr);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Rpm),
                            static_cast<uint8_t>(config.dash_tiles[0].parameter));
    TEST_ASSERT_EQUAL_UINT8(3U, config.dash_tiles[0].decimals);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 0.0f, config.dash_tiles[0].warning.hysteresis_native);
    TEST_ASSERT_EQUAL_UINT16(10000U,
                             config.dash_tiles[0].warning.delay_ms);
    TEST_ASSERT_EQUAL_CHAR('\0', config.can.profile_id.back());
}

void test_validation_rejects_invalid_shift_order_without_reordering_it() {
    AppConfig config = AppConfig::defaults();
    config.shift = ShiftLightConfig{7000U, 6000U, 5000U, 5000U, true};

    const ValidationResult result = config.validate();

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_FALSE(result.shift_order_valid);
    TEST_ASSERT_EQUAL_UINT16(7000U, config.shift.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(6000U, config.shift.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(5000U, config.shift.max_rpm);
}

void test_validation_rejects_red_zone_equal_to_maximum() {
    AppConfig config = AppConfig::defaults();
    config.shift = ShiftLightConfig{5500U, 7000U, 7000U, 7000U, true};

    const ValidationResult result = config.validate();

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_FALSE(result.shift_order_valid);
}

void test_validation_accepts_flash_equal_to_maximum() {
    AppConfig config = AppConfig::defaults();
    config.shift.start_rpm = 5500U;
    config.shift.red_rpm = 7000U;
    config.shift.flash_rpm = 8000U;
    config.shift.max_rpm = 8000U;

    const ValidationResult result = config.validate();

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_TRUE(result.shift_order_valid);
}

void test_validation_rejects_flash_not_above_red_zone() {
    AppConfig config = AppConfig::defaults();
    config.shift.start_rpm = 5500U;
    config.shift.red_rpm = 7000U;
    config.shift.flash_rpm = 7000U;
    config.shift.max_rpm = 8000U;

    const ValidationResult result = config.validate();

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_FALSE(result.shift_order_valid);
}

void test_validation_rejects_unsnapped_or_out_of_range_shift_values() {
    AppConfig unsnapped = AppConfig::defaults();
    unsnapped.shift =
        ShiftLightConfig{5500U, 5501U, 5502U, 5502U, true};
    TEST_ASSERT_FALSE(unsnapped.validate().valid);

    AppConfig zero_start = AppConfig::defaults();
    zero_start.shift =
        ShiftLightConfig{0U, 100U, 200U, 200U, true};
    TEST_ASSERT_TRUE(zero_start.validate().valid);

    AppConfig above_range = AppConfig::defaults();
    above_range.shift =
        ShiftLightConfig{9000U, 9500U, 10100U, 10100U, true};
    TEST_ASSERT_FALSE(above_range.validate().valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_define_approved_dash_and_track_slots);
    RUN_TEST(test_defaults_use_demo_metric_and_shared_shift_configuration);
    RUN_TEST(test_validation_normalizes_unsafe_persisted_values);
    RUN_TEST(test_validation_rejects_invalid_shift_order_without_reordering_it);
    RUN_TEST(test_validation_rejects_red_zone_equal_to_maximum);
    RUN_TEST(test_validation_accepts_flash_equal_to_maximum);
    RUN_TEST(test_validation_rejects_flash_not_above_red_zone);
    RUN_TEST(test_validation_rejects_unsnapped_or_out_of_range_shift_values);
    return UNITY_END();
}
