#include <unity.h>

#include "alarms/alarm_manager.h"
#include "alarms/warning_config.h"
#include "telemetry/parameter_registry.h"

namespace {
ParameterValue validValue(float value, uint32_t updated_ms = 0U) {
    ParameterValue result{};
    result.value = value;
    result.valid = true;
    result.updated_ms = updated_ms;
    return result;
}

WarningConfig highConfig() {
    WarningConfig cfg{};
    cfg.mode = WarningMode::High;
    cfg.warning_threshold = 100.0f;
    cfg.critical_threshold = 110.0f;
    cfg.hysteresis = 3.0f;
    return cfg;
}

void test_off_and_invalid_data() {
    AlarmManager alarms;
    WarningConfig cfg{};
    cfg.mode = WarningMode::Off;

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(150.0f), 0.0f, cfg, 100U)));

    cfg.mode = WarningMode::High;
    ParameterValue invalid{};
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Invalid),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, invalid, 0.0f, cfg, 101U)));
}

void test_high_warning_and_critical_thresholds() {
    AlarmManager alarms;
    const WarningConfig cfg = highConfig();

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(99.9f), 0.0f, cfg, 100U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(100.0f), 0.0f, cfg, 101U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(110.0f), 0.0f, cfg, 102U)));
}

void test_low_warning_and_critical_thresholds() {
    AlarmManager alarms;
    WarningConfig cfg{};
    cfg.mode = WarningMode::Low;
    cfg.warning_threshold = 2.0f;
    cfg.critical_threshold = 1.0f;

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::OilPressure, validValue(2.1f), 3000.0f, cfg, 100U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::OilPressure, validValue(2.0f), 3000.0f, cfg, 101U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::OilPressure, validValue(1.0f), 3000.0f, cfg, 102U)));
}

void test_range_warning_and_critical_bands() {
    AlarmManager alarms;
    WarningConfig cfg{};
    cfg.mode = WarningMode::Range;
    cfg.warning_low = 12.0f;
    cfg.warning_high = 15.0f;
    cfg.critical_low = 11.0f;
    cfg.critical_high = 16.0f;

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::BatteryVoltage, validValue(13.8f), 0.0f, cfg, 100U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::BatteryVoltage, validValue(11.8f), 0.0f, cfg, 101U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::BatteryVoltage, validValue(10.9f), 0.0f, cfg, 102U)));
    alarms.reset(ParameterId::BatteryVoltage);
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::BatteryVoltage, validValue(15.2f), 0.0f, cfg, 103U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::BatteryVoltage, validValue(16.1f), 0.0f, cfg, 104U)));
}

void test_entry_delay_requires_continuous_violation() {
    AlarmManager alarms;
    WarningConfig cfg = highConfig();
    cfg.delay_ms = 300U;

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(105.0f), 0.0f, cfg, 1000U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(105.0f), 0.0f, cfg, 1299U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(105.0f), 0.0f, cfg, 1300U)));

    alarms.reset(ParameterId::Clt);
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(105.0f), 0.0f, cfg, 2000U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(95.0f), 0.0f, cfg, 2100U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(105.0f), 0.0f, cfg, 2200U)));
}

void test_hysteresis_prevents_chatter_when_clearing_high_alarm() {
    AlarmManager alarms;
    WarningConfig cfg = highConfig();

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(103.0f), 0.0f, cfg, 100U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(98.0f), 0.0f, cfg, 101U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(96.9f), 0.0f, cfg, 102U)));

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(112.0f), 0.0f, cfg, 103U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(108.0f), 0.0f, cfg, 104U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::Clt, validValue(106.9f), 0.0f, cfg, 105U)));
}

void test_rpm_curve_interpolates_and_clamps_thresholds() {
    AlarmManager alarms;
    WarningConfig cfg{};
    cfg.mode = WarningMode::RpmCurve;
    cfg.rpm_curve_count = 3;
    cfg.rpm_curve[0] = {1000.0f, 1.0f, 0.6f};
    cfg.rpm_curve[1] = {3000.0f, 2.0f, 1.2f};
    cfg.rpm_curve[2] = {6000.0f, 3.5f, 2.2f};

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, alarms.rpmWarningThreshold(cfg, 500.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.6f, alarms.rpmCriticalThreshold(cfg, 500.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, alarms.rpmWarningThreshold(cfg, 2000.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.9f, alarms.rpmCriticalThreshold(cfg, 2000.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5f, alarms.rpmWarningThreshold(cfg, 7000.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.2f, alarms.rpmCriticalThreshold(cfg, 7000.0f));
}

void test_rpm_curve_drives_oil_pressure_alarm() {
    AlarmManager alarms;
    WarningConfig cfg{};
    cfg.mode = WarningMode::RpmCurve;
    cfg.rpm_curve_count = 2;
    cfg.rpm_curve[0] = {1000.0f, 1.0f, 0.5f};
    cfg.rpm_curve[1] = {5000.0f, 3.0f, 1.5f};

    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Normal),
                      static_cast<int>(alarms.evaluate(ParameterId::OilPressure, validValue(2.2f), 3000.0f, cfg, 100U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Warning),
                      static_cast<int>(alarms.evaluate(ParameterId::OilPressure, validValue(1.8f), 3000.0f, cfg, 101U)));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::OilPressure, validValue(0.9f), 3000.0f, cfg, 102U)));
}

void test_alarm_evaluation_is_independent_from_picker_visibility() {
    ParameterRegistry registry;
    registry.setPickerVisible(ParameterId::OilPressure, false);
    registry.set(ParameterId::OilPressure, 0.7f, 100U);

    AlarmManager alarms;
    WarningConfig cfg{};
    cfg.mode = WarningMode::Low;
    cfg.warning_threshold = 1.5f;
    cfg.critical_threshold = 0.8f;

    TEST_ASSERT_FALSE(registry.pickerVisible(ParameterId::OilPressure));
    TEST_ASSERT_EQUAL(static_cast<int>(AlarmState::Critical),
                      static_cast<int>(alarms.evaluate(ParameterId::OilPressure,
                                                       registry.value(ParameterId::OilPressure),
                                                       2500.0f, cfg, 100U)));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_off_and_invalid_data);
    RUN_TEST(test_high_warning_and_critical_thresholds);
    RUN_TEST(test_low_warning_and_critical_thresholds);
    RUN_TEST(test_range_warning_and_critical_bands);
    RUN_TEST(test_entry_delay_requires_continuous_violation);
    RUN_TEST(test_hysteresis_prevents_chatter_when_clearing_high_alarm);
    RUN_TEST(test_rpm_curve_interpolates_and_clamps_thresholds);
    RUN_TEST(test_rpm_curve_drives_oil_pressure_alarm);
    RUN_TEST(test_alarm_evaluation_is_independent_from_picker_visibility);
    return UNITY_END();
}
