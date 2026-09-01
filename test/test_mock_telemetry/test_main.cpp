#include <unity.h>
#include <cmath>
#include "telemetry/mock_telemetry.h"

static void assert_ranges(const VehicleState& s) {
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(900, s.rpm);
    TEST_ASSERT_LESS_OR_EQUAL_UINT16(7800, s.rpm);
    TEST_ASSERT_GREATER_OR_EQUAL_INT8(1, s.gear);
    TEST_ASSERT_LESS_OR_EQUAL_INT8(6, s.gear);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, std::fmax(0.0f, std::fmin(190.0f, s.speed_kph)), s.speed_kph);
    TEST_ASSERT_TRUE(s.map_bar >= 0.35f && s.map_bar <= 1.50f);
    TEST_ASSERT_TRUE(s.lambda >= 0.78f && s.lambda <= 1.05f);
    TEST_ASSERT_TRUE(s.clt_c >= 80.0f && s.clt_c <= 103.0f);
    TEST_ASSERT_TRUE(s.iat_c >= 25.0f && s.iat_c <= 55.0f);
    TEST_ASSERT_TRUE(s.oil_pressure_bar >= 1.2f && s.oil_pressure_bar <= 5.8f);
    TEST_ASSERT_TRUE(s.oil_temp_c >= 75.0f && s.oil_temp_c <= 120.0f);
    TEST_ASSERT_TRUE(s.fuel_pressure_bar >= 3.0f && s.fuel_pressure_bar <= 4.5f);
    TEST_ASSERT_TRUE(s.battery_v >= 12.8f && s.battery_v <= 14.4f);
    TEST_ASSERT_TRUE(s.tps_percent >= 0.0f && s.tps_percent <= 100.0f);
}

void test_reset_has_valid_idle_state() {
    MockTelemetry mock;
    mock.reset();
    const auto& s = mock.state();
    TEST_ASSERT_EQUAL_UINT16(900, s.rpm);
    TEST_ASSERT_EQUAL_INT8(1, s.gear);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.speed_kph);
    assert_ranges(s);
}

void test_same_timestamp_is_deterministic() {
    MockTelemetry a;
    MockTelemetry b;
    a.update(12345);
    b.update(12345);
    const auto& x = a.state();
    const auto& y = b.state();
    TEST_ASSERT_EQUAL_UINT16(x.rpm, y.rpm);
    TEST_ASSERT_EQUAL_INT8(x.gear, y.gear);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.map_bar, y.map_bar);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.lambda, y.lambda);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.tps_percent, y.tps_percent);
}

void test_values_stay_in_engineering_ranges() {
    MockTelemetry mock;
    for (uint32_t t = 0; t <= 120000; t += 137) {
        mock.update(t);
        assert_ranges(mock.state());
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_reset_has_valid_idle_state);
    RUN_TEST(test_same_timestamp_is_deterministic);
    RUN_TEST(test_values_stay_in_engineering_ranges);
    return UNITY_END();
}
