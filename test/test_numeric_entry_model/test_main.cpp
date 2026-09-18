#include <unity.h>
#if __has_include("ui/numeric_entry_model.h")
#include "ui/numeric_entry_model.h"
void test_replacing_and_erasing_number() {
    NumericEntryModel m; m.open(95, 0, 999, 1);
    m.append('1'); m.append('2'); m.append('0'); m.append('.'); m.append('5');
    TEST_ASSERT_TRUE(m.valid()); TEST_ASSERT_FLOAT_WITHIN(.001f,120.5f,m.value());
    m.erase(); TEST_ASSERT_FLOAT_WITHIN(.001f,120,m.value());
}
void test_precision_and_range_are_enforced() {
    NumericEntryModel m; m.open(0,0,999,1);
    for(char c : {'9','9','9','.','0','1'}) m.append(c);
    TEST_ASSERT_EQUAL_STRING("999.0",m.text()); TEST_ASSERT_TRUE(m.valid());
    m.open(0,0,999,1); for(char c : {'1','0','0','0'}) m.append(c);
    TEST_ASSERT_FALSE(m.valid());
}
void test_negative_and_empty_input() {
    NumericEntryModel m; m.open(20,-999,999,1);
    m.append('-'); TEST_ASSERT_FALSE(m.valid()); m.append('5');
    TEST_ASSERT_TRUE(m.valid()); TEST_ASSERT_FLOAT_WITHIN(.001f,-5,m.value());
    m.erase(); m.erase(); TEST_ASSERT_FALSE(m.valid());
}
void test_duplicate_separator_and_integer_field() {
    NumericEntryModel m; m.open(1,0,999,1);
    m.append('1');m.append('.');m.append('.');m.append('2');
    TEST_ASSERT_EQUAL_STRING("1.2",m.text());
    m.open(8000,100,10000,0);m.append('7');m.append('.');m.append('5');
    TEST_ASSERT_EQUAL_STRING("75",m.text());
}
#else
void test_replacing_and_erasing_number() { TEST_FAIL_MESSAGE("Numeric entry is not implemented"); }
void test_precision_and_range_are_enforced() { TEST_FAIL_MESSAGE("Numeric entry is not implemented"); }
void test_negative_and_empty_input() { TEST_FAIL_MESSAGE("Numeric entry is not implemented"); }
void test_duplicate_separator_and_integer_field() { TEST_FAIL_MESSAGE("Numeric entry is not implemented"); }
#endif
int main(int,char**) { UNITY_BEGIN(); RUN_TEST(test_replacing_and_erasing_number);
 RUN_TEST(test_precision_and_range_are_enforced);RUN_TEST(test_negative_and_empty_input);
 RUN_TEST(test_duplicate_separator_and_integer_field);return UNITY_END(); }
