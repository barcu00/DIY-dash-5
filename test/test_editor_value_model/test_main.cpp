#include <unity.h>
#if __has_include("ui/editor_value_model.h")
#include "ui/editor_value_model.h"
void test_reset_temperature_uses_absolute_conversion() {
    UnitSettings u;u.temperature=TemperatureUnit::Fahrenheit;
    TileWarningConfig w;
    TEST_ASSERT_TRUE(warningFromPresented(ParameterId::OilTemperature,u,true,
        WarningDirection::Above,248,239,1000,w));
    TEST_ASSERT_FLOAT_WITHIN(.001f,120,w.threshold_native);
    TEST_ASSERT_FLOAT_WITHIN(.001f,5,w.hysteresis_native);
}
void test_reset_below_and_above_are_validated() {
    UnitSettings u;TileWarningConfig w;
    TEST_ASSERT_FALSE(warningFromPresented(ParameterId::OilPressure,u,true,
        WarningDirection::Above,4,5,0,w));
    TEST_ASSERT_TRUE(warningFromPresented(ParameterId::OilPressure,u,true,
        WarningDirection::Below,1,1.5f,0,w));
    TEST_ASSERT_FLOAT_WITHIN(.001f,.5f,w.hysteresis_native);
}
void test_category_mapping() {
    TEST_ASSERT_EQUAL(1,static_cast<int>(parameterCategory(ParameterId::Clt)));
    TEST_ASSERT_EQUAL(2,static_cast<int>(parameterCategory(ParameterId::OilPressure)));
    TEST_ASSERT_EQUAL(3,static_cast<int>(parameterCategory(ParameterId::CheckEngine)));
    TEST_ASSERT_EQUAL(0,static_cast<int>(parameterCategory(ParameterId::Rpm)));
}
#else
void test_reset_temperature_uses_absolute_conversion() {TEST_FAIL_MESSAGE("Reset-value editing missing");}
void test_reset_below_and_above_are_validated() {TEST_FAIL_MESSAGE("Reset-value validation missing");}
void test_category_mapping() {TEST_FAIL_MESSAGE("Parameter categories missing");}
#endif
int main(int,char**) {UNITY_BEGIN();RUN_TEST(test_reset_temperature_uses_absolute_conversion);
 RUN_TEST(test_reset_below_and_above_are_validated);RUN_TEST(test_category_mapping);return UNITY_END();}
