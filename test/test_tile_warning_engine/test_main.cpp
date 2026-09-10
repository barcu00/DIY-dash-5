#include <optional>

#include <unity.h>

// Production changes caught: warnings firing before delay, ignoring hysteresis,
// retriggering after acknowledgement, or hidden/multiple warnings being lost.

#include "alarms/tile_warning_engine.h"

namespace {
AppConfig oneWarning(PageId page, uint8_t slot, ParameterId parameter,
                     WarningDirection direction, float threshold,
                     float hysteresis, uint16_t delay_ms, bool visible = true) {
    AppConfig config = AppConfig::defaults();
    for (TileConfig& tile : config.dash_tiles) tile.warning.enabled = false;
    for (TileConfig& tile : config.track_tiles) tile.warning.enabled = false;
    TileConfig& tile = page == PageId::Dash ? config.dash_tiles[slot]
                                              : config.track_tiles[slot];
    tile.parameter = parameter;
    tile.visible = visible;
    tile.warning.enabled = true;
    tile.warning.direction = direction;
    tile.warning.threshold_native = threshold;
    tile.warning.hysteresis_native = hysteresis;
    tile.warning.delay_ms = delay_ms;
    return config;
}

bool sameAddress(TileAddress left, TileAddress right) {
    return left.page == right.page && left.slot == right.slot;
}
}  // namespace

void test_above_warning_activates_only_after_configured_delay() {
    const AppConfig config = oneWarning(PageId::Dash, 0U, ParameterId::Clt,
        WarningDirection::Above, 110.0f, 5.0f, 200U);
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(ParameterId::Clt, 111.0f, 100U);
    TileWarningEngine engine;

    engine.evaluate(config, state, 100U);
    TEST_ASSERT_FALSE(engine.nextModal().has_value());
    engine.evaluate(config, state, 299U);
    TEST_ASSERT_FALSE(engine.nextModal().has_value());
    engine.evaluate(config, state, 300U);

    TEST_ASSERT_TRUE(engine.nextModal().has_value());
    TEST_ASSERT_TRUE(engine.isHighlighted({PageId::Dash, 0U}));
}

void test_below_warning_uses_mirrored_hysteresis_rearm() {
    const AppConfig config = oneWarning(PageId::Dash, 1U,
        ParameterId::OilPressure, WarningDirection::Below, 1.0f, 0.2f, 0U);
    VehicleState state;
    state.reset(DataSource::Can);
    TileWarningEngine engine;

    state.set(ParameterId::OilPressure, 0.9f, 0U);
    engine.evaluate(config, state, 0U);
    TEST_ASSERT_TRUE(engine.nextModal().has_value());
    engine.acknowledge({PageId::Dash, 1U});

    state.set(ParameterId::OilPressure, 1.1f, 10U);
    engine.evaluate(config, state, 10U);
    TEST_ASSERT_TRUE(engine.isHighlighted({PageId::Dash, 1U}));
    state.set(ParameterId::OilPressure, 1.2f, 20U);
    engine.evaluate(config, state, 20U);
    TEST_ASSERT_FALSE(engine.isHighlighted({PageId::Dash, 1U}));
}

void test_acknowledged_warning_retriggers_only_after_safe_rearm() {
    const AppConfig config = oneWarning(PageId::Dash, 0U, ParameterId::Clt,
        WarningDirection::Above, 110.0f, 5.0f, 0U);
    VehicleState state;
    state.reset(DataSource::Can);
    TileWarningEngine engine;

    state.set(ParameterId::Clt, 111.0f, 0U);
    engine.evaluate(config, state, 0U);
    TEST_ASSERT_TRUE(engine.nextModal().has_value());
    engine.acknowledge({PageId::Dash, 0U});
    engine.evaluate(config, state, 10U);
    TEST_ASSERT_FALSE(engine.nextModal().has_value());
    TEST_ASSERT_TRUE(engine.isHighlighted({PageId::Dash, 0U}));

    state.set(ParameterId::Clt, 104.0f, 20U);
    engine.evaluate(config, state, 20U);
    TEST_ASSERT_FALSE(engine.isHighlighted({PageId::Dash, 0U}));
    state.set(ParameterId::Clt, 111.0f, 30U);
    engine.evaluate(config, state, 30U);
    TEST_ASSERT_TRUE(engine.nextModal().has_value());
}

void test_invalid_value_clears_pending_and_active_warning() {
    const AppConfig config = oneWarning(PageId::Dash, 0U, ParameterId::Clt,
        WarningDirection::Above, 110.0f, 5.0f, 100U);
    VehicleState state;
    state.reset(DataSource::Can);
    TileWarningEngine engine;

    state.set(ParameterId::Clt, 120.0f, 0U);
    engine.evaluate(config, state, 0U);
    state.invalidate(ParameterId::Clt);
    engine.evaluate(config, state, 100U);
    TEST_ASSERT_FALSE(engine.nextModal().has_value());
    TEST_ASSERT_FALSE(engine.isHighlighted({PageId::Dash, 0U}));
}

void test_hidden_tile_warning_still_opens_global_modal() {
    const AppConfig config = oneWarning(PageId::Track, 2U, ParameterId::Iat,
        WarningDirection::Above, 60.0f, 2.0f, 0U, false);
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(ParameterId::Iat, 70.0f, 0U);
    TileWarningEngine engine;

    engine.evaluate(config, state, 0U);
    const std::optional<WarningModalData> modal = engine.nextModal();

    TEST_ASSERT_TRUE(modal.has_value());
    TEST_ASSERT_TRUE(sameAddress({PageId::Track, 2U}, modal->address));
}

void test_largest_normalized_excursion_is_shown_first() {
    AppConfig config = oneWarning(PageId::Dash, 0U, ParameterId::Clt,
        WarningDirection::Above, 100.0f, 1.0f, 0U);
    config.track_tiles[0].parameter = ParameterId::Rpm;
    config.track_tiles[0].warning =
        TileWarningConfig{true, WarningDirection::Above, 5000.0f, 100.0f, 0U};
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(ParameterId::Clt, 110.0f, 0U);
    state.set(ParameterId::Rpm, 7000.0f, 0U);
    TileWarningEngine engine;

    engine.evaluate(config, state, 0U);
    const std::optional<WarningModalData> first = engine.nextModal();

    TEST_ASSERT_TRUE(first.has_value());
    TEST_ASSERT_TRUE(sameAddress({PageId::Track, 0U}, first->address));
    TEST_ASSERT_EQUAL_UINT8(1U, first->remaining_count);
    engine.acknowledge(first->address);
    const std::optional<WarningModalData> second = engine.nextModal();
    TEST_ASSERT_TRUE(sameAddress({PageId::Dash, 0U}, second->address));
    TEST_ASSERT_EQUAL_UINT8(0U, second->remaining_count);
}

void test_equal_excursions_use_page_then_slot_order() {
    AppConfig config = oneWarning(PageId::Dash, 3U, ParameterId::Clt,
        WarningDirection::Above, 100.0f, 1.0f, 0U);
    config.dash_tiles[1].parameter = ParameterId::Clt;
    config.dash_tiles[1].warning = config.dash_tiles[3].warning;
    config.track_tiles[0].parameter = ParameterId::Clt;
    config.track_tiles[0].warning = config.dash_tiles[3].warning;
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(ParameterId::Clt, 110.0f, 0U);
    TileWarningEngine engine;

    engine.evaluate(config, state, 0U);
    const std::optional<WarningModalData> modal = engine.nextModal();

    TEST_ASSERT_TRUE(sameAddress({PageId::Dash, 1U}, modal->address));
    TEST_ASSERT_EQUAL_UINT8(2U, modal->remaining_count);
}

void test_flag_tiles_never_open_numeric_warning_modal() {
    const AppConfig config = oneWarning(
        PageId::Dash, 0U, ParameterId::CheckEngine,
        WarningDirection::Above, 0.5f, 0.0f, 0U);
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(ParameterId::CheckEngine, 1.0f, 0U);
    TileWarningEngine engine;

    engine.evaluate(config, state, 0U);

    TEST_ASSERT_FALSE(engine.nextModal().has_value());
    TEST_ASSERT_FALSE(engine.isHighlighted({PageId::Dash, 0U}));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_above_warning_activates_only_after_configured_delay);
    RUN_TEST(test_below_warning_uses_mirrored_hysteresis_rearm);
    RUN_TEST(test_acknowledged_warning_retriggers_only_after_safe_rearm);
    RUN_TEST(test_invalid_value_clears_pending_and_active_warning);
    RUN_TEST(test_hidden_tile_warning_still_opens_global_modal);
    RUN_TEST(test_largest_normalized_excursion_is_shown_first);
    RUN_TEST(test_equal_excursions_use_page_then_slot_order);
    RUN_TEST(test_flag_tiles_never_open_numeric_warning_modal);
    return UNITY_END();
}
