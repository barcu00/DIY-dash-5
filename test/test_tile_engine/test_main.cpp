#include <cstddef>

#include <unity.h>

// Production changes caught: hidden tiles leaving top/bottom gaps, crossing a
// logical group, changing order, or wide DASH tiles failing to compact.

#include "ui/tile_engine.h"

namespace {
const TilePlacement* findPlacement(const TilePlacementList& placements,
                                   PageId page, uint8_t slot) {
    for (std::size_t i = 0U; i < placements.count; ++i) {
        if (placements.items[i].address.page == page &&
            placements.items[i].address.slot == slot) {
            return &placements.items[i];
        }
    }
    return nullptr;
}

std::size_t countGroup(const TilePlacementList& placements, TileGroup group) {
    std::size_t count = 0U;
    for (std::size_t i = 0U; i < placements.count; ++i) {
        if (placements.items[i].group == group) {
            ++count;
        }
    }
    return count;
}
}  // namespace

void test_all_visible_layouts_emit_every_logical_tile() {
    const AppConfig config = AppConfig::defaults();

    TEST_ASSERT_EQUAL_UINT32(
        14U, TileEngine::placements(PageId::Dash, config).count);
    TEST_ASSERT_EQUAL_UINT32(
        12U, TileEngine::placements(PageId::Track, config).count);
}

void test_dash_left_tiles_compact_to_lowest_rows_without_crossing_groups() {
    AppConfig config = AppConfig::defaults();
    config.dash_tiles[0].visible = false;

    const TilePlacementList placements =
        TileEngine::placements(PageId::Dash, config);

    TEST_ASSERT_EQUAL_UINT32(3U,
        countGroup(placements, TileGroup::DashLeft));
    TEST_ASSERT_EQUAL_INT16(134,
        findPlacement(placements, PageId::Dash, 1U)->geometry.y);
    TEST_ASSERT_EQUAL_INT16(232,
        findPlacement(placements, PageId::Dash, 2U)->geometry.y);
    TEST_ASSERT_EQUAL_INT16(332,
        findPlacement(placements, PageId::Dash, 3U)->geometry.y);
    TEST_ASSERT_EQUAL_UINT32(4U,
        countGroup(placements, TileGroup::DashRight));
    TEST_ASSERT_EQUAL_INT16(8,
        findPlacement(placements, PageId::Dash, 1U)->geometry.x);
}

void test_dash_wide_tiles_compact_only_inside_their_two_wide_positions() {
    AppConfig config = AppConfig::defaults();
    config.dash_tiles[4].visible = false;

    const TilePlacementList placements =
        TileEngine::placements(PageId::Dash, config);
    const TilePlacement* gear = findPlacement(placements, PageId::Dash, 5U);

    TEST_ASSERT_NOT_NULL(gear);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TileGroup::DashCenterWide),
                            static_cast<uint8_t>(gear->group));
    TEST_ASSERT_EQUAL_INT16(192, gear->geometry.x);
    TEST_ASSERT_EQUAL_INT16(134, gear->geometry.y);
    TEST_ASSERT_EQUAL_INT16(416, gear->geometry.width);
    TEST_ASSERT_EQUAL_INT16(90, gear->geometry.height);
}

void test_dash_center_small_tiles_bottom_pack_in_row_major_order() {
    AppConfig config = AppConfig::defaults();
    config.dash_tiles[6].visible = false;

    const TilePlacementList placements =
        TileEngine::placements(PageId::Dash, config);
    const TilePlacement* first_visible =
        findPlacement(placements, PageId::Dash, 7U);
    const TilePlacement* last_visible =
        findPlacement(placements, PageId::Dash, 9U);

    TEST_ASSERT_EQUAL_INT16(404, first_visible->geometry.x);
    TEST_ASSERT_EQUAL_INT16(232, first_visible->geometry.y);
    TEST_ASSERT_EQUAL_INT16(404, last_visible->geometry.x);
    TEST_ASSERT_EQUAL_INT16(332, last_visible->geometry.y);
}

void test_track_center_tiles_compact_to_bottom_and_keep_order() {
    AppConfig config = AppConfig::defaults();
    config.track_tiles[4].visible = false;
    config.track_tiles[6].visible = false;

    const TilePlacementList placements =
        TileEngine::placements(PageId::Track, config);
    const TilePlacement* first =
        findPlacement(placements, PageId::Track, 5U);
    const TilePlacement* second =
        findPlacement(placements, PageId::Track, 7U);

    TEST_ASSERT_EQUAL_UINT32(2U,
        countGroup(placements, TileGroup::TrackCenter));
    TEST_ASSERT_EQUAL_INT16(232, first->geometry.y);
    TEST_ASSERT_EQUAL_INT16(332, second->geometry.y);
    TEST_ASSERT_EQUAL_INT16(416, first->geometry.width);
}

void test_all_hidden_page_produces_no_placements() {
    AppConfig config = AppConfig::defaults();
    for (TileConfig& tile : config.track_tiles) {
        tile.visible = false;
    }

    TEST_ASSERT_EQUAL_UINT32(
        0U, TileEngine::placements(PageId::Track, config).count);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_all_visible_layouts_emit_every_logical_tile);
    RUN_TEST(test_dash_left_tiles_compact_to_lowest_rows_without_crossing_groups);
    RUN_TEST(test_dash_wide_tiles_compact_only_inside_their_two_wide_positions);
    RUN_TEST(test_dash_center_small_tiles_bottom_pack_in_row_major_order);
    RUN_TEST(test_track_center_tiles_compact_to_bottom_and_keep_order);
    RUN_TEST(test_all_hidden_page_produces_no_placements);
    return UNITY_END();
}
