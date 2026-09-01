#include <cstring>
#include <unity.h>

#include "ui/tile_editor_model.h"

namespace {
void test_parameter_selection_updates_tile() {
    TileConfig tile{};
    TileEditorModel::setParameter(tile, ParameterId::OilPressure);
    TEST_ASSERT_EQUAL(static_cast<int>(ParameterId::OilPressure), static_cast<int>(tile.parameter));
}

void test_visibility_can_be_toggled() {
    TileConfig tile{};
    TileEditorModel::setVisible(tile, false);
    TEST_ASSERT_FALSE(tile.visible);
    TileEditorModel::setVisible(tile, true);
    TEST_ASSERT_TRUE(tile.visible);
}

void test_custom_label_can_be_set_and_cleared() {
    TileConfig tile{};
    TileEditorModel::setCustomLabel(tile, "OIL P");
    TEST_ASSERT_TRUE(tile.custom_label_enabled);
    TEST_ASSERT_EQUAL_STRING("OIL P", tile.custom_label.data());

    TileEditorModel::clearCustomLabel(tile);
    TEST_ASSERT_FALSE(tile.custom_label_enabled);
    TEST_ASSERT_EQUAL_STRING("", tile.custom_label.data());
}

void test_icon_modes_default_custom_and_none() {
    TileConfig tile{};

    TileEditorModel::useDefaultIcon(tile);
    TEST_ASSERT_TRUE(tile.icon_enabled);
    TEST_ASSERT_EQUAL_UINT16(0U, tile.icon);

    TileEditorModel::useCustomIcon(tile, IconId::OilPressure);
    TEST_ASSERT_TRUE(tile.icon_enabled);
    TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(IconId::OilPressure), tile.icon);

    TileEditorModel::disableIcon(tile);
    TEST_ASSERT_FALSE(tile.icon_enabled);
}

void test_afr_mode_is_allowed_only_for_lambda_parameters() {
    TileConfig lambda{};
    lambda.parameter = ParameterId::Lambda;
    TEST_ASSERT_TRUE(TileEditorModel::setAfrMode(lambda, true));
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Afr), static_cast<int>(lambda.value_format));

    TileConfig target{};
    target.parameter = ParameterId::LambdaTarget;
    TEST_ASSERT_TRUE(TileEditorModel::setAfrMode(target, true));
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Afr), static_cast<int>(target.value_format));

    TileConfig speed{};
    speed.parameter = ParameterId::VehicleSpeed;
    TEST_ASSERT_FALSE(TileEditorModel::setAfrMode(speed, true));
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Native), static_cast<int>(speed.value_format));
}

void test_changing_away_from_lambda_resets_afr_mode() {
    TileConfig tile{};
    tile.parameter = ParameterId::Lambda;
    TEST_ASSERT_TRUE(TileEditorModel::setAfrMode(tile, true));
    TileEditorModel::setParameter(tile, ParameterId::OilTemperature);
    TEST_ASSERT_EQUAL(static_cast<int>(ValueFormatMode::Native), static_cast<int>(tile.value_format));
}

void test_label_is_truncated_and_null_terminated() {
    TileConfig tile{};
    TileEditorModel::setCustomLabel(tile, "THIS LABEL IS MUCH LONGER THAN THE BUFFER");
    TEST_ASSERT_TRUE(tile.custom_label_enabled);
    TEST_ASSERT_EQUAL_CHAR('\0', tile.custom_label.back());
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(tile.custom_label.size() - 1U, std::strlen(tile.custom_label.data()));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_parameter_selection_updates_tile);
    RUN_TEST(test_visibility_can_be_toggled);
    RUN_TEST(test_custom_label_can_be_set_and_cleared);
    RUN_TEST(test_icon_modes_default_custom_and_none);
    RUN_TEST(test_afr_mode_is_allowed_only_for_lambda_parameters);
    RUN_TEST(test_changing_away_from_lambda_resets_afr_mode);
    RUN_TEST(test_label_is_truncated_and_null_terminated);
    return UNITY_END();
}
