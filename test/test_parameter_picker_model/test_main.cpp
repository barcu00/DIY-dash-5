#include <unity.h>

#include "ui/parameter_picker_model.h"

namespace {
void test_next_visible_skips_hidden_parameters() {
    AppConfig cfg = AppConfig::defaults();
    cfg.parameter_visible.fill(false);
    cfg.parameter_visible[parameterIndex(ParameterId::Rpm)] = true;
    cfg.parameter_visible[parameterIndex(ParameterId::OilPressure)] = true;

    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::OilPressure),
                      static_cast<int>(ParameterPickerModel::nextVisible(cfg, ParameterId::Rpm)));
}

void test_previous_visible_wraps() {
    AppConfig cfg = AppConfig::defaults();
    cfg.parameter_visible.fill(false);
    cfg.parameter_visible[parameterIndex(ParameterId::Rpm)] = true;
    cfg.parameter_visible[parameterIndex(ParameterId::OilPressure)] = true;

    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::OilPressure),
                      static_cast<int>(ParameterPickerModel::previousVisible(cfg, ParameterId::Rpm)));
}

void test_current_is_returned_when_it_is_only_visible_parameter() {
    AppConfig cfg = AppConfig::defaults();
    cfg.parameter_visible.fill(false);
    cfg.parameter_visible[parameterIndex(ParameterId::Lambda)] = true;

    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Lambda),
                      static_cast<int>(ParameterPickerModel::nextVisible(cfg, ParameterId::Lambda)));
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::Lambda),
                      static_cast<int>(ParameterPickerModel::previousVisible(cfg, ParameterId::Lambda)));
}

void test_visibility_can_be_changed_without_touching_tile_visibility() {
    AppConfig cfg = AppConfig::defaults();
    const bool original_tile_visibility = cfg.dash_tiles[0][0].visible;
    ParameterPickerModel::setVisible(cfg, ParameterId::VehicleSpeed, false);

    TEST_ASSERT_FALSE(cfg.parameter_visible[parameterIndex(ParameterId::VehicleSpeed)]);
    TEST_ASSERT_EQUAL(original_tile_visibility, cfg.dash_tiles[0][0].visible);
}

void test_at_least_one_picker_parameter_is_preserved() {
    AppConfig cfg = AppConfig::defaults();
    cfg.parameter_visible.fill(false);
    cfg.parameter_visible[parameterIndex(ParameterId::Rpm)] = true;

    TEST_ASSERT_FALSE(ParameterPickerModel::setVisible(cfg, ParameterId::Rpm, false));
    TEST_ASSERT_TRUE(cfg.parameter_visible[parameterIndex(ParameterId::Rpm)]);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_next_visible_skips_hidden_parameters);
    RUN_TEST(test_previous_visible_wraps);
    RUN_TEST(test_current_is_returned_when_it_is_only_visible_parameter);
    RUN_TEST(test_visibility_can_be_changed_without_touching_tile_visibility);
    RUN_TEST(test_at_least_one_picker_parameter_is_preserved);
    return UNITY_END();
}
