#include <cstring>

#include <unity.h>

// Production changes caught: the bar not taking the shift-light colours at
// YELLOW FROM / RED FROM, flash not filling and blinking the whole bar, or
// invalid RPM leaving a stale fill.

#include "ui/gauge_model.h"
#include "ui/tach_bar_model.h"

namespace {
ShiftLightConfig shift() {
    ShiftLightConfig config;
    config.start_rpm = 5500U;
    config.red_rpm = 7000U;
    config.flash_rpm = 7500U;
    config.flash_enabled = true;
    return config;
}
}  // namespace

void test_colour_follows_shift_thresholds() {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TachColor::Normal),
        static_cast<uint8_t>(tachBarState(3000.0f, true, 8000U, shift(), 0U).color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TachColor::Yellow),
        static_cast<uint8_t>(tachBarState(5500.0f, true, 8000U, shift(), 0U).color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TachColor::Red),
        static_cast<uint8_t>(tachBarState(7000.0f, true, 8000U, shift(), 0U).color));
    TEST_ASSERT_EQUAL_UINT16(500U, tachBarState(4000.0f, true, 8000U, shift(), 0U).fill_permille);
}

void test_flash_fills_and_blinks_whole_bar_at_4_hz() {
    const TachBarState on = tachBarState(7600.0f, true, 8000U, shift(), 0U);
    const TachBarState off = tachBarState(7600.0f, true, 8000U, shift(), 130U);
    TEST_ASSERT_TRUE(on.flashing);
    TEST_ASSERT_EQUAL_UINT16(1000U, on.fill_permille);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TachColor::Red), static_cast<uint8_t>(on.color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TachColor::Off), static_cast<uint8_t>(off.color));
    ShiftLightConfig no_flash = shift();
    no_flash.flash_enabled = false;
    TEST_ASSERT_FALSE(tachBarState(7600.0f, true, 8000U, no_flash, 0U).flashing);
}

void test_invalid_rpm_empties_the_bar() {
    const TachBarState state = tachBarState(6000.0f, false, 8000U, shift(), 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, state.fill_permille);
    TEST_ASSERT_FALSE(state.flashing);
}

void test_gauge_range_uses_temperature_bar_and_warning_side() {
    TileConfig oil;
    oil.parameter = ParameterId::OilPressure;
    oil.warning = {true, WarningDirection::Below, 1.0f, 0.2f, 0U};
    const GaugeRange pressure = gaugeRange(oil);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, pressure.maximum);
    TEST_ASSERT_EQUAL_UINT16(100U, pressure.red_below_permille);
    TEST_ASSERT_EQUAL_UINT16(500U, gaugeFill(5.0f, pressure));
    TEST_ASSERT_EQUAL_UINT16(1000U, gaugeFill(50.0f, pressure));

    TileConfig coolant;
    coolant.parameter = ParameterId::Clt;
    coolant.temperature_bar = {true, 40.0f, 75.0f, 115.0f, 140.0f};
    const GaugeRange temperature = gaugeRange(coolant);
    TEST_ASSERT_EQUAL_FLOAT(40.0f, temperature.minimum);
    TEST_ASSERT_EQUAL_FLOAT(140.0f, temperature.maximum);
    TEST_ASSERT_EQUAL_UINT16(750U, temperature.red_from_permille);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_colour_follows_shift_thresholds);
    RUN_TEST(test_flash_fills_and_blinks_whole_bar_at_4_hz);
    RUN_TEST(test_invalid_rpm_empties_the_bar);
    RUN_TEST(test_gauge_range_uses_temperature_bar_and_warning_side);
    return UNITY_END();
}
