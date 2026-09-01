#include <unity.h>

#include "alarms/alarm_runtime.h"
#include "settings/app_config.h"
#include "telemetry/parameter_registry.h"

void setUp() {}
void tearDown() {}

namespace {
void test_runtime_evaluates_configured_warning_from_registry() {
    ParameterRegistry registry;
    AppConfig config = AppConfig::defaults();
    AlarmManager manager;
    AlarmRuntime runtime;

    WarningConfig& w = config.warnings[parameterIndex(ParameterId::Clt)];
    w.mode = WarningMode::High;
    w.warning_threshold = 100.0f;
    w.critical_threshold = 110.0f;
    w.delay_ms = 0U;

    registry.set(ParameterId::Rpm, 3000.0f, 10U);
    registry.set(ParameterId::Clt, 115.0f, 10U);
    runtime.update(registry, config, manager, 10U);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::Critical),
                          static_cast<int>(manager.state(ParameterId::Clt)));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::Critical),
                          static_cast<int>(runtime.highestState()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ParameterId::Clt),
                          static_cast<int>(runtime.highestParameter()));
}

void test_runtime_keeps_hidden_parameter_warning_active() {
    ParameterRegistry registry;
    AppConfig config = AppConfig::defaults();
    AlarmManager manager;
    AlarmRuntime runtime;

    config.parameter_visible[parameterIndex(ParameterId::OilPressure)] = false;
    WarningConfig& w = config.warnings[parameterIndex(ParameterId::OilPressure)];
    w.mode = WarningMode::Low;
    w.warning_threshold = 2.0f;
    w.critical_threshold = 1.0f;
    w.delay_ms = 0U;

    registry.set(ParameterId::Rpm, 4000.0f, 20U);
    registry.set(ParameterId::OilPressure, 0.8f, 20U);
    runtime.update(registry, config, manager, 20U);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::Critical),
                          static_cast<int>(manager.state(ParameterId::OilPressure)));
}

void test_invalid_values_clear_runtime_alarm() {
    ParameterRegistry registry;
    AppConfig config = AppConfig::defaults();
    AlarmManager manager;
    AlarmRuntime runtime;

    WarningConfig& w = config.warnings[parameterIndex(ParameterId::Clt)];
    w.mode = WarningMode::High;
    w.warning_threshold = 90.0f;
    w.critical_threshold = 100.0f;
    w.delay_ms = 0U;

    registry.set(ParameterId::Clt, 110.0f, 1U);
    runtime.update(registry, config, manager, 1U);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::Critical), static_cast<int>(runtime.highestState()));

    registry.invalidateAll();
    runtime.update(registry, config, manager, 2U);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::Normal), static_cast<int>(runtime.highestState()));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_runtime_evaluates_configured_warning_from_registry);
    RUN_TEST(test_runtime_keeps_hidden_parameter_warning_active);
    RUN_TEST(test_invalid_values_clear_runtime_alarm);
    return UNITY_END();
}
