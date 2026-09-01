#include <unity.h>

#include "ui/warning_editor_model.h"

namespace {
void test_mode_and_scalar_thresholds_are_editable() {
    WarningConfig cfg{};
    WarningEditorModel::setMode(cfg, WarningMode::High);
    WarningEditorModel::setHighThresholds(cfg, 105.0f, 115.0f);
    WarningEditorModel::setHysteresis(cfg, 3.0f);
    WarningEditorModel::setDelayMs(cfg, 750U);

    TEST_ASSERT_EQUAL(static_cast<int>(WarningMode::High), static_cast<int>(cfg.mode));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 105.0f, cfg.warning_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 115.0f, cfg.critical_threshold);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, cfg.hysteresis);
    TEST_ASSERT_EQUAL_UINT32(750U, cfg.delay_ms);
}

void test_range_thresholds_are_normalized() {
    WarningConfig cfg{};
    WarningEditorModel::setRangeThresholds(cfg, 1.2f, 0.8f, 1.3f, 0.7f);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.8f, cfg.warning_low);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.2f, cfg.warning_high);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.7f, cfg.critical_low);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, cfg.critical_high);
}

void test_negative_hysteresis_is_clamped_to_zero() {
    WarningConfig cfg{};
    WarningEditorModel::setHysteresis(cfg, -4.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cfg.hysteresis);
}

void test_rpm_points_are_sorted_by_rpm() {
    WarningConfig cfg{};
    TEST_ASSERT_TRUE(WarningEditorModel::addRpmPoint(cfg, 6000.0f, 3.0f, 2.5f));
    TEST_ASSERT_TRUE(WarningEditorModel::addRpmPoint(cfg, 1000.0f, 1.0f, 0.6f));
    TEST_ASSERT_TRUE(WarningEditorModel::addRpmPoint(cfg, 3000.0f, 2.0f, 1.5f));

    TEST_ASSERT_EQUAL_UINT8(3U, cfg.rpm_curve_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1000.0f, cfg.rpm_curve[0].rpm);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3000.0f, cfg.rpm_curve[1].rpm);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6000.0f, cfg.rpm_curve[2].rpm);
}

void test_rpm_curve_is_limited_to_eight_points() {
    WarningConfig cfg{};
    for (uint8_t i = 0; i < WarningConfig::kMaxRpmCurvePoints; ++i) {
        TEST_ASSERT_TRUE(WarningEditorModel::addRpmPoint(cfg, 1000.0f + i * 500.0f, 1.0f, 0.5f));
    }
    TEST_ASSERT_FALSE(WarningEditorModel::addRpmPoint(cfg, 7000.0f, 4.0f, 3.0f));
    TEST_ASSERT_EQUAL_UINT8(WarningConfig::kMaxRpmCurvePoints, cfg.rpm_curve_count);
}

void test_rpm_point_can_be_updated_and_removed() {
    WarningConfig cfg{};
    TEST_ASSERT_TRUE(WarningEditorModel::addRpmPoint(cfg, 1000.0f, 1.0f, 0.5f));
    TEST_ASSERT_TRUE(WarningEditorModel::addRpmPoint(cfg, 3000.0f, 2.0f, 1.5f));

    TEST_ASSERT_TRUE(WarningEditorModel::updateRpmPoint(cfg, 1U, 2500.0f, 2.2f, 1.7f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2500.0f, cfg.rpm_curve[1].rpm);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.2f, cfg.rpm_curve[1].warning);

    TEST_ASSERT_TRUE(WarningEditorModel::removeRpmPoint(cfg, 0U));
    TEST_ASSERT_EQUAL_UINT8(1U, cfg.rpm_curve_count);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2500.0f, cfg.rpm_curve[0].rpm);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_mode_and_scalar_thresholds_are_editable);
    RUN_TEST(test_range_thresholds_are_normalized);
    RUN_TEST(test_negative_hysteresis_is_clamped_to_zero);
    RUN_TEST(test_rpm_points_are_sorted_by_rpm);
    RUN_TEST(test_rpm_curve_is_limited_to_eight_points);
    RUN_TEST(test_rpm_point_can_be_updated_and_removed);
    return UNITY_END();
}
