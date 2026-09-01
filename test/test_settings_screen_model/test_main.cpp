#include <unity.h>

#include "ui/settings_screen_model.h"

namespace {
void test_settings_sections_match_v02_plan() {
    TEST_ASSERT_EQUAL_UINT8(7U, SettingsScreenModel::sectionCount());
    TEST_ASSERT_EQUAL_STRING("CAN", SettingsScreenModel::sectionLabel(0));
    TEST_ASSERT_EQUAL_STRING("DASH LAYOUT", SettingsScreenModel::sectionLabel(1));
    TEST_ASSERT_EQUAL_STRING("TRACK LAYOUT", SettingsScreenModel::sectionLabel(2));
    TEST_ASSERT_EQUAL_STRING("PARAMETERS", SettingsScreenModel::sectionLabel(3));
    TEST_ASSERT_EQUAL_STRING("WARNINGS", SettingsScreenModel::sectionLabel(4));
    TEST_ASSERT_EQUAL_STRING("FUEL / AFR", SettingsScreenModel::sectionLabel(5));
    TEST_ASSERT_EQUAL_STRING("SYSTEM", SettingsScreenModel::sectionLabel(6));
}

void test_invalid_section_index_returns_empty_label() {
    TEST_ASSERT_EQUAL_STRING("", SettingsScreenModel::sectionLabel(7));
    TEST_ASSERT_EQUAL_STRING("", SettingsScreenModel::sectionLabel(255));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_settings_sections_match_v02_plan);
    RUN_TEST(test_invalid_section_index_returns_empty_label);
    return UNITY_END();
}
