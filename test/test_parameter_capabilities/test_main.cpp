#include <cstddef>
#include <initializer_list>

#include <unity.h>

#include "ecu/can_profile_registry.h"
#include "ecu/parameter_capabilities.h"

void test_demo_supports_every_registered_parameter_once() {
    const ParameterCapabilities capabilities =
        ParameterCapabilities::forSource(DataSource::Demo, nullptr);

    TEST_ASSERT_EQUAL_UINT32(parameterCount(), capabilities.count());
    for (std::size_t index = 0U; index < parameterCount(); ++index) {
        const ParameterId expected = static_cast<ParameterId>(index);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected),
                                static_cast<uint8_t>(capabilities.at(index)));
        TEST_ASSERT_TRUE(capabilities.supports(expected));
    }
}

void test_ms43_capabilities_are_derived_from_compiled_signals() {
    const ParameterCapabilities capabilities =
        ParameterCapabilities::forSource(
            DataSource::Can, CanProfileRegistry::find("bmw_ms43_stock"));

    TEST_ASSERT_EQUAL_UINT32(22U, capabilities.count());
    TEST_ASSERT_TRUE(capabilities.supports(ParameterId::Rpm));
    TEST_ASSERT_TRUE(capabilities.supports(ParameterId::OilTemperature));
    TEST_ASSERT_TRUE(capabilities.supports(ParameterId::CheckEngine));
    TEST_ASSERT_TRUE(capabilities.supports(ParameterId::EngineRunning));
    TEST_ASSERT_TRUE(capabilities.supports(ParameterId::LowOilPressure));
    TEST_ASSERT_FALSE(capabilities.supports(ParameterId::Lambda));
    TEST_ASSERT_FALSE(capabilities.supports(ParameterId::AntiLagActive));
}

void test_profiles_without_documented_flags_remain_numeric_only() {
    for (const char* profile_id : {
             "haltech_broadcast_2_0",
             "speeduino_haltech",
             "psa_c2_vts_engine_experimental",
         }) {
        const ParameterCapabilities capabilities =
            ParameterCapabilities::forSource(
                DataSource::Can, CanProfileRegistry::find(profile_id));
        for (std::size_t index = 0U; index < capabilities.count(); ++index) {
            TEST_ASSERT_EQUAL_UINT8(
                static_cast<uint8_t>(ParameterKind::Numeric),
                static_cast<uint8_t>(
                    parameterDescriptor(capabilities.at(index)).kind));
        }
    }
}

void test_null_can_profile_and_invalid_parameter_are_safe() {
    const ParameterCapabilities capabilities =
        ParameterCapabilities::forSource(DataSource::Can, nullptr);
    TEST_ASSERT_EQUAL_UINT32(0U, capabilities.count());
    TEST_ASSERT_FALSE(capabilities.supports(ParameterId::Rpm));
    TEST_ASSERT_FALSE(
        capabilities.supports(static_cast<ParameterId>(255U)));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(ParameterId::Count),
        static_cast<uint8_t>(capabilities.at(0U)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_demo_supports_every_registered_parameter_once);
    RUN_TEST(test_ms43_capabilities_are_derived_from_compiled_signals);
    RUN_TEST(test_profiles_without_documented_flags_remain_numeric_only);
    RUN_TEST(test_null_can_profile_and_invalid_parameter_are_safe);
    return UNITY_END();
}
