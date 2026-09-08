#include <unity.h>

#include "ui/tile_refresh_policy.h"

void test_fast_driving_signals_render_at_panel_cadence() {
    TEST_ASSERT_EQUAL_UINT32(25U, tileRefreshIntervalMs(ParameterId::Rpm));
    TEST_ASSERT_EQUAL_UINT32(25U, tileRefreshIntervalMs(ParameterId::Speed));
    TEST_ASSERT_EQUAL_UINT32(25U, tileRefreshIntervalMs(ParameterId::Map));
    TEST_ASSERT_EQUAL_UINT32(25U, tileRefreshIntervalMs(ParameterId::Tps));
    TEST_ASSERT_EQUAL_UINT32(25U, tileRefreshIntervalMs(ParameterId::WheelSpeedRf));
}

void test_medium_combustion_signals_render_at_twenty_hertz() {
    TEST_ASSERT_EQUAL_UINT32(50U, tileRefreshIntervalMs(ParameterId::Lambda));
    TEST_ASSERT_EQUAL_UINT32(50U, tileRefreshIntervalMs(ParameterId::OilPressure));
    TEST_ASSERT_EQUAL_UINT32(50U, tileRefreshIntervalMs(ParameterId::IgnitionTiming));
    TEST_ASSERT_EQUAL_UINT32(50U, tileRefreshIntervalMs(ParameterId::Gear));
}

void test_slow_thermal_signals_render_at_ten_hertz() {
    TEST_ASSERT_EQUAL_UINT32(100U, tileRefreshIntervalMs(ParameterId::Clt));
    TEST_ASSERT_EQUAL_UINT32(100U, tileRefreshIntervalMs(ParameterId::BatteryVoltage));
    TEST_ASSERT_EQUAL_UINT32(100U, tileRefreshIntervalMs(ParameterId::Egt8));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_fast_driving_signals_render_at_panel_cadence);
    RUN_TEST(test_medium_combustion_signals_render_at_twenty_hertz);
    RUN_TEST(test_slow_thermal_signals_render_at_ten_hertz);
    return UNITY_END();
}
