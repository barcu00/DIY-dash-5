#include <unity.h>

#include "ui/settings_flow_model.h"

void test_navigation_starts_home_and_returns_home() {
    SettingsFlowModel model;

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SettingsCategory::Home),
                            static_cast<uint8_t>(model.category()));
    model.open(SettingsCategory::DataCan);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SettingsCategory::DataCan),
                            static_cast<uint8_t>(model.category()));
    model.backToHome();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SettingsCategory::Home),
                            static_cast<uint8_t>(model.category()));
}

void test_layout_pages_are_six_slots_and_page_is_clamped() {
    SettingsFlowModel model;

    model.selectLayout(PageId::Dash);
    TEST_ASSERT_EQUAL_UINT32(3U, model.pageCount());
    TEST_ASSERT_EQUAL_UINT32(0U, model.firstSlot());
    TEST_ASSERT_TRUE(model.nextPage());
    TEST_ASSERT_EQUAL_UINT32(6U, model.firstSlot());
    TEST_ASSERT_TRUE(model.nextPage());
    TEST_ASSERT_EQUAL_UINT32(12U, model.firstSlot());
    TEST_ASSERT_FALSE(model.nextPage());

    model.selectLayout(PageId::Track);
    TEST_ASSERT_EQUAL_UINT32(2U, model.pageCount());
    TEST_ASSERT_EQUAL_UINT32(0U, model.pageIndex());
}

void test_discrete_changes_commit_immediately_and_sliders_on_release() {
    TEST_ASSERT_TRUE(SettingsFlowModel::shouldPersist(
        SettingsInputKind::Discrete, SettingsInputEvent::ValueChanged));
    TEST_ASSERT_FALSE(SettingsFlowModel::shouldPersist(
        SettingsInputKind::Slider, SettingsInputEvent::ValueChanged));
    TEST_ASSERT_TRUE(SettingsFlowModel::shouldPersist(
        SettingsInputKind::Slider, SettingsInputEvent::Released));
}

void test_start_change_pushes_later_shift_thresholds_up() {
    const ShiftLightConfig corrected = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 8000U}, ShiftField::Start, 7900U);

    TEST_ASSERT_EQUAL_UINT16(7900U, corrected.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(8000U, corrected.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(8100U, corrected.max_rpm);
}

void test_maximum_change_pulls_earlier_shift_thresholds_down() {
    const ShiftLightConfig corrected = SettingsFlowModel::correctedShift(
        ShiftLightConfig{7900U, 8000U, 8100U}, ShiftField::Maximum, 6000U);

    TEST_ASSERT_EQUAL_UINT16(5800U, corrected.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(5900U, corrected.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(6000U, corrected.max_rpm);
}

void test_shift_changes_clamp_to_supported_range() {
    const ShiftLightConfig high = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 8000U}, ShiftField::Start, 16000U);
    TEST_ASSERT_EQUAL_UINT16(14800U, high.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(14900U, high.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(15000U, high.max_rpm);

    const ShiftLightConfig low = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 8000U}, ShiftField::Maximum, 500U);
    TEST_ASSERT_EQUAL_UINT16(1000U, low.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(1100U, low.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(1200U, low.max_rpm);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_navigation_starts_home_and_returns_home);
    RUN_TEST(test_layout_pages_are_six_slots_and_page_is_clamped);
    RUN_TEST(test_discrete_changes_commit_immediately_and_sliders_on_release);
    RUN_TEST(test_start_change_pushes_later_shift_thresholds_up);
    RUN_TEST(test_maximum_change_pulls_earlier_shift_thresholds_down);
    RUN_TEST(test_shift_changes_clamp_to_supported_range);
    return UNITY_END();
}
