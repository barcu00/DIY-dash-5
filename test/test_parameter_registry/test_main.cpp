#include <unity.h>

#include "telemetry/parameter_registry.h"
#include "ui/icon_id.h"

void test_registry_descriptor_and_value_lifecycle() {
    ParameterRegistry registry;

    TEST_ASSERT_EQUAL_STRING("RPM", registry.descriptor(ParameterId::Rpm).short_name);
    TEST_ASSERT_FALSE(registry.value(ParameterId::Rpm).valid);

    registry.set(ParameterId::Rpm, 4321.0f, 1000U);
    const auto& value = registry.value(ParameterId::Rpm);
    TEST_ASSERT_TRUE(value.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4321.0f, value.value);
    TEST_ASSERT_EQUAL_UINT32(1000U, value.updated_ms);

    registry.invalidateAll();
    TEST_ASSERT_FALSE(registry.value(ParameterId::Rpm).valid);
}

void test_registry_picker_visibility_is_independent() {
    ParameterRegistry registry;

    TEST_ASSERT_TRUE(registry.pickerVisible(ParameterId::Rpm));
    registry.setPickerVisible(ParameterId::Rpm, false);
    TEST_ASSERT_FALSE(registry.pickerVisible(ParameterId::Rpm));
    registry.setPickerVisible(ParameterId::Rpm, true);
    TEST_ASSERT_TRUE(registry.pickerVisible(ParameterId::Rpm));
}

void test_descriptors_expose_catalog_default_icons() {
    ParameterRegistry registry;

    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Rpm),
                      static_cast<int>(registry.descriptor(ParameterId::Rpm).default_icon));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Boost),
                      static_cast<int>(registry.descriptor(ParameterId::Map).default_icon));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::OilPressure),
                      static_cast<int>(registry.descriptor(ParameterId::OilPressure).default_icon));
    TEST_ASSERT_EQUAL(static_cast<int>(IconId::Warning),
                      static_cast<int>(registry.descriptor(ParameterId::EcuError).default_icon));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_descriptor_and_value_lifecycle);
    RUN_TEST(test_registry_picker_visibility_is_independent);
    RUN_TEST(test_descriptors_expose_catalog_default_icons);
    return UNITY_END();
}
