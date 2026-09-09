#include <cstddef>
#include <initializer_list>

#include <unity.h>

// Production change caught: an omitted descriptor or an incorrect stable
// parameter-to-metadata mapping.

#include "telemetry/parameter_registry.h"

void test_registry_describes_every_stable_parameter() {
    TEST_ASSERT_TRUE(parameterCount() > 35U);

    for (std::size_t i = 0U; i < parameterCount(); ++i) {
        const auto id = static_cast<ParameterId>(i);
        const ParameterDescriptor& descriptor = parameterDescriptor(id);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(id),
                                static_cast<uint8_t>(descriptor.id));
        TEST_ASSERT_NOT_NULL(descriptor.name);
        TEST_ASSERT_NOT_NULL(descriptor.short_name);
        TEST_ASSERT_TRUE(descriptor.name[0] != '\0');
        TEST_ASSERT_TRUE(descriptor.short_name[0] != '\0');
    }
}

void test_numeric_ordinals_are_stable_and_flags_are_appended() {
    TEST_ASSERT_EQUAL_UINT8(0U, static_cast<uint8_t>(ParameterId::Rpm));
    TEST_ASSERT_EQUAL_UINT8(34U,
                            static_cast<uint8_t>(ParameterId::WheelSpeedRr));
    TEST_ASSERT_TRUE(static_cast<uint8_t>(ParameterId::IgnitionOn) >= 35U);

    const ParameterDescriptor& rpm = parameterDescriptor(ParameterId::Rpm);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterKind::Numeric),
                            static_cast<uint8_t>(rpm.kind));

    for (const ParameterId flag : {
             ParameterId::IgnitionOn,
             ParameterId::EngineRunning,
             ParameterId::CheckEngine,
             ParameterId::AntiLagActive,
             ParameterId::LaunchControlActive,
             ParameterId::LowOilPressure,
         }) {
        const ParameterDescriptor& descriptor = parameterDescriptor(flag);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterKind::Flag),
                                static_cast<uint8_t>(descriptor.kind));
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::None),
                                static_cast<uint8_t>(descriptor.native_unit));
        TEST_ASSERT_EQUAL_UINT8(0U, descriptor.default_decimals);
    }
}

void test_registry_preserves_core_names_units_and_decimals() {
    const ParameterDescriptor& rpm = parameterDescriptor(ParameterId::Rpm);
    TEST_ASSERT_EQUAL_STRING("Engine speed", rpm.name);
    TEST_ASSERT_EQUAL_STRING("RPM", rpm.short_name);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::Rpm),
                            static_cast<uint8_t>(rpm.native_unit));
    TEST_ASSERT_EQUAL_UINT8(0U, rpm.default_decimals);

    const ParameterDescriptor& oil =
        parameterDescriptor(ParameterId::OilPressure);
    TEST_ASSERT_EQUAL_STRING("Oil pressure", oil.name);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::Bar),
                            static_cast<uint8_t>(oil.native_unit));
    TEST_ASSERT_EQUAL_UINT8(1U, oil.default_decimals);

    const ParameterDescriptor& ignition =
        parameterDescriptor(ParameterId::IgnitionTiming);
    TEST_ASSERT_EQUAL_STRING("Ignition timing", ignition.name);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::Degrees),
                            static_cast<uint8_t>(ignition.native_unit));

    const ParameterDescriptor& pulse =
        parameterDescriptor(ParameterId::InjectorPulseWidth);
    TEST_ASSERT_EQUAL_STRING("INJ PW", pulse.short_name);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::Milliseconds),
                            static_cast<uint8_t>(pulse.native_unit));

    const ParameterDescriptor& airflow =
        parameterDescriptor(ParameterId::MassAirFlow);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::GramsPerSecond),
                            static_cast<uint8_t>(airflow.native_unit));

    TEST_ASSERT_EQUAL_STRING(
        "Wheel speed LF",
        parameterDescriptor(ParameterId::WheelSpeedLf).name);
    TEST_ASSERT_EQUAL_STRING(
        "EGT 8", parameterDescriptor(ParameterId::Egt8).short_name);
}

void test_registry_returns_unknown_descriptor_for_invalid_id() {
    const ParameterDescriptor& descriptor =
        parameterDescriptor(static_cast<ParameterId>(255U));

    TEST_ASSERT_EQUAL_STRING("Unknown", descriptor.name);
    TEST_ASSERT_EQUAL_STRING("---", descriptor.short_name);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(NativeUnit::None),
                            static_cast<uint8_t>(descriptor.native_unit));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_describes_every_stable_parameter);
    RUN_TEST(test_numeric_ordinals_are_stable_and_flags_are_appended);
    RUN_TEST(test_registry_preserves_core_names_units_and_decimals);
    RUN_TEST(test_registry_returns_unknown_descriptor_for_invalid_id);
    return UNITY_END();
}
