#include <unity.h>

// Production changes caught: edits leaking before Save, Cancel mutating config,
// a parameter change retaining an old alarm, or one slot overwriting another.

#include "ui/tile_editor_model.h"

void test_open_rejects_settings_page_and_out_of_range_slots() {
    const AppConfig config = AppConfig::defaults();
    TileEditorModel editor;

    TEST_ASSERT_FALSE(editor.open({PageId::Settings, 0U}, config));
    TEST_ASSERT_FALSE(editor.open({PageId::Dash, 14U}, config));
    TEST_ASSERT_FALSE(editor.open({PageId::Track, 12U}, config));
}

void test_cancel_discards_staged_visibility_and_parameter_changes() {
    AppConfig config = AppConfig::defaults();
    const ParameterId original = config.dash_tiles[0].parameter;
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 0U}, config));

    editor.setVisible(false);
    editor.setParameter(ParameterId::Clt);
    editor.cancel();

    TEST_ASSERT_FALSE(editor.isOpen());
    TEST_ASSERT_TRUE(config.dash_tiles[0].visible);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(original),
                            static_cast<uint8_t>(config.dash_tiles[0].parameter));
}

void test_apply_changes_only_selected_tile() {
    AppConfig config = AppConfig::defaults();
    const TileConfig untouched = config.dash_tiles[1];
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 0U}, config));
    editor.setVisible(false);
    editor.setDecimals(2U);

    TEST_ASSERT_TRUE(editor.applyTo(config));

    TEST_ASSERT_FALSE(config.dash_tiles[0].visible);
    TEST_ASSERT_EQUAL_UINT8(2U, config.dash_tiles[0].decimals);
    TEST_ASSERT_EQUAL_MEMORY(&untouched, &config.dash_tiles[1],
                             sizeof(TileConfig));
}

void test_parameter_change_disables_previous_warning_on_apply() {
    AppConfig config = AppConfig::defaults();
    config.dash_tiles[0].warning.enabled = true;
    config.dash_tiles[0].warning.threshold_native = 120.0f;
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 0U}, config));

    editor.setParameter(ParameterId::Clt);
    TEST_ASSERT_TRUE(editor.applyTo(config));

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::Clt),
                            static_cast<uint8_t>(config.dash_tiles[0].parameter));
    TEST_ASSERT_FALSE(config.dash_tiles[0].warning.enabled);
}

void test_same_parameter_preserves_edited_warning() {
    AppConfig config = AppConfig::defaults();
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Track, 4U}, config));
    TileWarningConfig warning;
    warning.enabled = true;
    warning.direction = WarningDirection::Above;
    warning.threshold_native = 900.0f;
    warning.hysteresis_native = 200.0f;
    warning.delay_ms = 250U;

    editor.setWarning(warning);
    TEST_ASSERT_TRUE(editor.applyTo(config));

    TEST_ASSERT_TRUE(config.track_tiles[4].warning.enabled);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 900.0f, config.track_tiles[4].warning.threshold_native);
    TEST_ASSERT_EQUAL_UINT16(250U, config.track_tiles[4].warning.delay_ms);
}

void test_apply_preserves_temperature_bar_settings_for_selected_tile() {
    AppConfig config = AppConfig::defaults();
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 3U}, config));
    const TemperatureBarConfig bar{true, 30.0f, 72.5f, 118.0f};

    editor.setTemperatureBar(bar);
    TEST_ASSERT_TRUE(editor.applyTo(config));

    TEST_ASSERT_TRUE(config.dash_tiles[3].temperature_bar.enabled);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 30.0f, config.dash_tiles[3].temperature_bar.minimum_native);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 72.5f, config.dash_tiles[3].temperature_bar.ready_native);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 118.0f, config.dash_tiles[3].temperature_bar.maximum_native);
}

void test_parameter_change_uses_safe_temperature_bar_defaults() {
    AppConfig config = AppConfig::defaults();
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 0U}, config));

    editor.setParameter(ParameterId::Clt);
    TEST_ASSERT_TRUE(editor.applyTo(config));

    TEST_ASSERT_TRUE(config.dash_tiles[0].temperature_bar.enabled);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 40.0f, config.dash_tiles[0].temperature_bar.minimum_native);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 75.0f, config.dash_tiles[0].temperature_bar.ready_native);
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 130.0f, config.dash_tiles[0].temperature_bar.maximum_native);
}

void test_apply_rejects_invalid_draft_without_mutating_config() {
    AppConfig config = AppConfig::defaults();
    const TileConfig original = config.track_tiles[0];
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Track, 0U}, config));
    editor.setParameter(static_cast<ParameterId>(255U));

    TEST_ASSERT_FALSE(editor.applyTo(config));
    TEST_ASSERT_EQUAL_MEMORY(&original, &config.track_tiles[0],
                             sizeof(TileConfig));
}

void test_flag_color_is_staged_saved_and_canceled_per_tile() {
    AppConfig config = AppConfig::defaults();
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 0U}, config));
    editor.setFlagActiveColor(FlagActiveColor::Red);
    TEST_ASSERT_TRUE(editor.applyTo(config));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(FlagActiveColor::Red),
                            static_cast<uint8_t>(
                                config.dash_tiles[0].flag_active_color));

    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 0U}, config));
    editor.setFlagActiveColor(FlagActiveColor::Green);
    editor.cancel();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(FlagActiveColor::Red),
                            static_cast<uint8_t>(
                                config.dash_tiles[0].flag_active_color));
}

void test_switching_to_flag_preserves_dormant_numeric_settings() {
    AppConfig config = AppConfig::defaults();
    TileConfig& tile = config.dash_tiles[3];
    tile.warning = {true, WarningDirection::Above, 105.5f, 2.5f, 400U};
    tile.temperature_bar = {true, 35.0f, 72.5f, 125.0f};
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 3U}, config));

    editor.setParameter(ParameterId::CheckEngine);
    TEST_ASSERT_TRUE(editor.applyTo(config));

    TEST_ASSERT_TRUE(tile.warning.enabled);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 105.5f,
                             tile.warning.threshold_native);
    TEST_ASSERT_TRUE(tile.temperature_bar.enabled);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 72.5f,
                             tile.temperature_bar.ready_native);
}

void test_candidate_preview_keeps_editor_open_and_runtime_unchanged() {
    AppConfig runtime = AppConfig::defaults();
    const TileConfig original = runtime.dash_tiles[0];
    TileEditorModel editor;
    TEST_ASSERT_TRUE(editor.open({PageId::Dash, 0U}, runtime));
    editor.setParameter(ParameterId::CheckEngine);
    editor.setFlagActiveColor(FlagActiveColor::Red);

    AppConfig candidate = runtime;
    TEST_ASSERT_TRUE(editor.writeCandidate(candidate));

    TEST_ASSERT_TRUE(editor.isOpen());
    TEST_ASSERT_EQUAL_MEMORY(&original, &runtime.dash_tiles[0],
                             sizeof(TileConfig));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ParameterId::CheckEngine),
                            static_cast<uint8_t>(
                                candidate.dash_tiles[0].parameter));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_open_rejects_settings_page_and_out_of_range_slots);
    RUN_TEST(test_cancel_discards_staged_visibility_and_parameter_changes);
    RUN_TEST(test_apply_changes_only_selected_tile);
    RUN_TEST(test_parameter_change_disables_previous_warning_on_apply);
    RUN_TEST(test_same_parameter_preserves_edited_warning);
    RUN_TEST(test_apply_preserves_temperature_bar_settings_for_selected_tile);
    RUN_TEST(test_parameter_change_uses_safe_temperature_bar_defaults);
    RUN_TEST(test_apply_rejects_invalid_draft_without_mutating_config);
    RUN_TEST(test_flag_color_is_staged_saved_and_canceled_per_tile);
    RUN_TEST(test_switching_to_flag_preserves_dormant_numeric_settings);
    RUN_TEST(test_candidate_preview_keeps_editor_open_and_runtime_unchanged);
    return UNITY_END();
}
