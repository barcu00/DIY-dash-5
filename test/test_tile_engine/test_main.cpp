#include <unity.h>

#include <array>

#include "settings/app_config.h"
#include "ui/tile_engine.h"

namespace {
void test_hidden_tiles_compact_without_gaps() {
    AppConfig config = AppConfig::defaults();
    auto& tiles = config.dash_tiles[0];
    tiles[1].visible = false;
    tiles[4].visible = false;
    tiles[8].visible = false;

    std::array<uint8_t, AppConfig::kTileCount> order{};
    const uint8_t count = TileEngine::visibleOrder(tiles, order);

    TEST_ASSERT_EQUAL_UINT8(9U, count);
    const uint8_t expected[] = {0, 2, 3, 5, 6, 7, 9, 10, 11};
    for (uint8_t i = 0; i < count; ++i) {
        TEST_ASSERT_EQUAL_UINT8(expected[i], order[i]);
        TEST_ASSERT_EQUAL_UINT8(i, TileEngine::compactSlot(tiles, expected[i]));
    }
}

void test_hidden_tile_has_no_compact_slot() {
    AppConfig config = AppConfig::defaults();
    auto& tiles = config.dash_tiles[0];
    tiles[3].visible = false;
    TEST_ASSERT_EQUAL_UINT8(TileEngine::kNoSlot, TileEngine::compactSlot(tiles, 3));
}

void test_lambda_to_afr_uses_configurable_stoich() {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 14.7f, TileEngine::lambdaToAfr(1.0f, 14.7f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 11.76f, TileEngine::lambdaToAfr(0.80f, 14.7f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.80f, TileEngine::lambdaToAfr(0.80f, 12.25f));
}

void test_present_value_switches_lambda_between_native_and_afr() {
    TileConfig tile{};
    tile.parameter = ParameterId::Lambda;
    tile.value_format = ValueFormatMode::Native;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.82f, TileEngine::presentValue(tile, 0.82f, 14.7f));

    tile.value_format = ValueFormatMode::Afr;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.054f, TileEngine::presentValue(tile, 0.82f, 14.7f));
}

void test_afr_mode_only_transforms_lambda_parameters() {
    TileConfig tile{};
    tile.parameter = ParameterId::OilPressure;
    tile.value_format = ValueFormatMode::Afr;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.4f, TileEngine::presentValue(tile, 3.4f, 14.7f));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_hidden_tiles_compact_without_gaps);
    RUN_TEST(test_hidden_tile_has_no_compact_slot);
    RUN_TEST(test_lambda_to_afr_uses_configurable_stoich);
    RUN_TEST(test_present_value_switches_lambda_between_native_and_afr);
    RUN_TEST(test_afr_mode_only_transforms_lambda_parameters);
    return UNITY_END();
}
