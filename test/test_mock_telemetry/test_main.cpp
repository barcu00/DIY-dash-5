#include <unity.h>
#include <cmath>
#include "telemetry/mock_telemetry.h"

static void assert_ranges(const VehicleState& s) {
    const float rpm = s.get(VehicleSignal::Rpm).value;
    const float gear = s.get(VehicleSignal::Gear).value;
    const float speed = s.get(VehicleSignal::Speed).value;
    TEST_ASSERT_TRUE(rpm >= 900.0f && rpm <= 7800.0f);
    TEST_ASSERT_TRUE(gear >= 1.0f && gear <= 6.0f);
    TEST_ASSERT_TRUE(speed >= 0.0f && speed <= 190.0f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::Map).value >= 0.35f && s.get(VehicleSignal::Map).value <= 1.50f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::Lambda).value >= 0.78f && s.get(VehicleSignal::Lambda).value <= 1.05f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::Clt).value >= 80.0f && s.get(VehicleSignal::Clt).value <= 103.0f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::Iat).value >= 25.0f && s.get(VehicleSignal::Iat).value <= 55.0f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::OilPressure).value >= 1.2f && s.get(VehicleSignal::OilPressure).value <= 5.8f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::OilTemperature).value >= 75.0f && s.get(VehicleSignal::OilTemperature).value <= 120.0f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::FuelPressure).value >= 3.0f && s.get(VehicleSignal::FuelPressure).value <= 4.5f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::BatteryVoltage).value >= 12.8f && s.get(VehicleSignal::BatteryVoltage).value <= 14.4f);
    TEST_ASSERT_TRUE(s.get(VehicleSignal::Tps).value >= 0.0f && s.get(VehicleSignal::Tps).value <= 100.0f);
}

void test_reset_has_valid_idle_state() {
    MockTelemetry mock;
    mock.reset();
    const auto& s = mock.state();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 900.0f, s.get(VehicleSignal::Rpm).value);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, s.get(VehicleSignal::Gear).value);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.get(VehicleSignal::Speed).value);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(s.source()));
    assert_ranges(s);
}

void test_same_timestamp_is_deterministic() {
    MockTelemetry a;
    MockTelemetry b;
    a.update(12345);
    b.update(12345);
    const auto& x = a.state();
    const auto& y = b.state();
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.get(VehicleSignal::Rpm).value, y.get(VehicleSignal::Rpm).value);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.get(VehicleSignal::Gear).value, y.get(VehicleSignal::Gear).value);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.get(VehicleSignal::Map).value, y.get(VehicleSignal::Map).value);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.get(VehicleSignal::Lambda).value, y.get(VehicleSignal::Lambda).value);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, x.get(VehicleSignal::Tps).value, y.get(VehicleSignal::Tps).value);
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
