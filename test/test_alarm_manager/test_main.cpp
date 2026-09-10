#include <unity.h>

#include "alarms/alarm_manager.h"

namespace {
VehicleState makeState() {
    VehicleState state;
    state.reset(DataSource::Can);
    return state;
}
}

void test_high_coolant_temperature_reaches_warning_and_critical() {
    AlarmManager alarms;
    VehicleState state = makeState();
    state.set(VehicleSignal::Clt, 110.0f, 1U);
    TEST_ASSERT_TRUE(alarms.evaluate(state).active(AlarmId::HighClt));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AlarmSeverity::Warning),
                            static_cast<uint8_t>(alarms.evaluate(state).severity));

    state.set(VehicleSignal::Clt, 120.0f, 2U);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AlarmSeverity::Critical),
                            static_cast<uint8_t>(alarms.evaluate(state).severity));
}

void test_temperature_oil_and_battery_thresholds_are_evaluated() {
    AlarmManager alarms;

    VehicleState iat = makeState();
    iat.set(VehicleSignal::Iat, 60.0f, 1U);
    TEST_ASSERT_TRUE(alarms.evaluate(iat).active(AlarmId::HighIat));

    VehicleState oil = makeState();
    oil.set(VehicleSignal::OilPressure, 0.7f, 1U);
    TEST_ASSERT_TRUE(alarms.evaluate(oil).active(AlarmId::LowOilPressure));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AlarmSeverity::Critical),
                            static_cast<uint8_t>(alarms.evaluate(oil).severity));

    VehicleState battery = makeState();
    battery.set(VehicleSignal::BatteryVoltage, 11.8f, 1U);
    TEST_ASSERT_TRUE(alarms.evaluate(battery).active(AlarmId::LowBattery));
}

void test_lean_lambda_requires_engine_load() {
    AlarmManager alarms;
    VehicleState state = makeState();
    state.set(VehicleSignal::Lambda, 1.10f, 1U);
    state.set(VehicleSignal::Map, 1.20f, 1U);
    state.set(VehicleSignal::Tps, 75.0f, 1U);
    TEST_ASSERT_TRUE(alarms.evaluate(state).active(AlarmId::LeanLambda));

    state.set(VehicleSignal::Map, 0.50f, 2U);
    state.set(VehicleSignal::Tps, 20.0f, 2U);
    TEST_ASSERT_FALSE(alarms.evaluate(state).active(AlarmId::LeanLambda));
}

void test_invalid_values_do_not_raise_measurement_alarms() {
    AlarmManager alarms;
    VehicleState state = makeState();
    state.set(VehicleSignal::Clt, 130.0f, 1U);
    state.invalidate(VehicleSignal::Clt);

    const AlarmSummary summary = alarms.evaluate(state);

    TEST_ASSERT_EQUAL_UINT32(0U, summary.active_mask);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AlarmSeverity::None),
                            static_cast<uint8_t>(summary.severity));
}

void test_each_alarm_preserves_its_own_severity() {
    AlarmManager alarms;
    VehicleState state = makeState();
    state.set(VehicleSignal::Clt, 110.0f, 1U);
    state.set(VehicleSignal::OilPressure, 0.7f, 1U);

    const AlarmSummary summary = alarms.evaluate(state);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AlarmSeverity::Warning),
                            static_cast<uint8_t>(summary.severityFor(AlarmId::HighClt)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AlarmSeverity::Critical),
                            static_cast<uint8_t>(summary.severityFor(AlarmId::LowOilPressure)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_high_coolant_temperature_reaches_warning_and_critical);
    RUN_TEST(test_temperature_oil_and_battery_thresholds_are_evaluated);
    RUN_TEST(test_lean_lambda_requires_engine_load);
    RUN_TEST(test_invalid_values_do_not_raise_measurement_alarms);
    RUN_TEST(test_each_alarm_preserves_its_own_severity);
    return UNITY_END();
}
