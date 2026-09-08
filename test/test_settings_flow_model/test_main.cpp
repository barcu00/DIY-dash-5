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

void test_start_change_pushes_later_shift_thresholds_up() {
    const ShiftLightConfig corrected = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 7500U, 8000U, true},
        ShiftField::Start, 7900U);

    TEST_ASSERT_EQUAL_UINT16(7900U, corrected.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(8000U, corrected.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(8100U, corrected.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(8100U, corrected.max_rpm);
}

void test_maximum_change_pulls_earlier_shift_thresholds_down() {
    const ShiftLightConfig corrected = SettingsFlowModel::correctedShift(
        ShiftLightConfig{7900U, 8000U, 8050U, 8100U, true},
        ShiftField::Maximum, 6000U);

    TEST_ASSERT_EQUAL_UINT16(5800U, corrected.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(5900U, corrected.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(6000U, corrected.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(6000U, corrected.max_rpm);
}

void test_flash_change_pulls_earlier_thresholds_down_without_moving_maximum() {
    const ShiftLightConfig corrected = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 7500U, 8000U, true},
        ShiftField::Flash, 6900U);

    TEST_ASSERT_EQUAL_UINT16(5500U, corrected.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(6800U, corrected.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(6900U, corrected.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(8000U, corrected.max_rpm);
}

void test_shift_changes_clamp_to_supported_range() {
    const ShiftLightConfig high = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 7500U, 8000U, true},
        ShiftField::Start, 16000U);
    TEST_ASSERT_EQUAL_UINT16(9800U, high.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(9900U, high.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(10000U, high.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(10000U, high.max_rpm);

    const ShiftLightConfig low = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 7500U, 8000U, true},
        ShiftField::Maximum, 500U);
    TEST_ASSERT_EQUAL_UINT16(300U, low.start_rpm);
    TEST_ASSERT_EQUAL_UINT16(400U, low.red_rpm);
    TEST_ASSERT_EQUAL_UINT16(500U, low.flash_rpm);
    TEST_ASSERT_EQUAL_UINT16(500U, low.max_rpm);
}

void test_slider_request_is_rounded_to_the_nearest_hundred_rpm() {
    const ShiftLightConfig corrected = SettingsFlowModel::correctedShift(
        ShiftLightConfig{5500U, 7000U, 7500U, 8000U, true},
        ShiftField::Flash, 7549U);

    TEST_ASSERT_EQUAL_UINT16(7500U, corrected.flash_rpm);
}

void test_reset_requires_explicit_request_and_can_be_cancelled() {
    SettingsFlowModel model;

    TEST_ASSERT_FALSE(model.resetPending());
    model.requestReset(SettingsResetTarget::TrackLayout);
    TEST_ASSERT_TRUE(model.resetPending());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SettingsResetTarget::TrackLayout),
                            static_cast<uint8_t>(model.pendingReset()));
    model.cancelReset();
    TEST_ASSERT_FALSE(model.resetPending());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_navigation_starts_home_and_returns_home);
    RUN_TEST(test_layout_pages_are_six_slots_and_page_is_clamped);
    RUN_TEST(test_start_change_pushes_later_shift_thresholds_up);
    RUN_TEST(test_maximum_change_pulls_earlier_shift_thresholds_down);
    RUN_TEST(test_flash_change_pulls_earlier_thresholds_down_without_moving_maximum);
    RUN_TEST(test_shift_changes_clamp_to_supported_range);
    RUN_TEST(test_slider_request_is_rounded_to_the_nearest_hundred_rpm);
    RUN_TEST(test_reset_requires_explicit_request_and_can_be_cancelled);
    return UNITY_END();
}
