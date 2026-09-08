#include <unity.h>

// Production changes caught: bars appearing on non-temperature tiles, invalid
// signals looking healthy, or fill/zone boundaries being calculated wrongly.

#include "ui/temperature_bar_model.h"

void test_disabled_and_non_temperature_bars_stay_hidden() {
    TemperatureBarConfig config{false, 40.0f, 75.0f, 130.0f};
    const SignalValue valid{86.0f, 0U, true};
    TEST_ASSERT_FALSE(
        temperatureBarState(ParameterId::Clt, valid, config).visible);

    config.enabled = true;
    TEST_ASSERT_FALSE(
        temperatureBarState(ParameterId::Rpm, valid, config).visible);
}

void test_invalid_temperature_shows_an_empty_unavailable_bar() {
    const TemperatureBarConfig config{true, 40.0f, 75.0f, 130.0f};
    const TemperatureBarState state = temperatureBarState(
        ParameterId::OilTemperature, SignalValue{}, config);

    TEST_ASSERT_TRUE(state.visible);
    TEST_ASSERT_EQUAL_UINT16(0U, state.fill_per_mille);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TemperatureBarZone::Unavailable),
                            static_cast<uint8_t>(state.zone));
}

void test_fill_is_clamped_and_ready_value_is_green() {
    const TemperatureBarConfig config{true, 40.0f, 75.0f, 130.0f};

    const TemperatureBarState below = temperatureBarState(
        ParameterId::Clt, SignalValue{20.0f, 0U, true}, config);
    const TemperatureBarState ready = temperatureBarState(
        ParameterId::Clt, SignalValue{75.0f, 0U, true}, config);
    const TemperatureBarState above = temperatureBarState(
        ParameterId::Clt, SignalValue{150.0f, 0U, true}, config);

    TEST_ASSERT_EQUAL_UINT16(0U, below.fill_per_mille);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TemperatureBarZone::Cold),
                            static_cast<uint8_t>(below.zone));
    TEST_ASSERT_EQUAL_UINT16(389U, ready.fill_per_mille);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TemperatureBarZone::Normal),
                            static_cast<uint8_t>(ready.zone));
    TEST_ASSERT_EQUAL_UINT16(1000U, above.fill_per_mille);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TemperatureBarZone::Hot),
                            static_cast<uint8_t>(above.zone));
}

void test_final_quarter_before_maximum_is_the_warm_zone() {
    const TemperatureBarConfig config{true, 40.0f, 80.0f, 120.0f};

    const TemperatureBarState normal = temperatureBarState(
        ParameterId::FuelTemperature, SignalValue{109.9f, 0U, true}, config);
    const TemperatureBarState warm = temperatureBarState(
        ParameterId::FuelTemperature, SignalValue{110.0f, 0U, true}, config);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TemperatureBarZone::Normal),
                            static_cast<uint8_t>(normal.zone));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TemperatureBarZone::Warm),
                            static_cast<uint8_t>(warm.zone));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_disabled_and_non_temperature_bars_stay_hidden);
    RUN_TEST(test_invalid_temperature_shows_an_empty_unavailable_bar);
    RUN_TEST(test_fill_is_clamped_and_ready_value_is_green);
    RUN_TEST(test_final_quarter_before_maximum_is_the_warm_zone);
    return UNITY_END();
}
