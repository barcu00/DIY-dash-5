#include "tile_engine.h"

#include <array>
#include "ui/dashboard_layout.h"

namespace {
using TileGeometryList = std::array<TileGeometry, 4>;

constexpr TileGeometry small(int16_t x, int16_t y, int16_t width) {
    return {x, y, width, TileLayout::kTileHeight, TileSize::Small};
}

constexpr TileGeometry wide(int16_t y) {
    return {TileLayout::kCenterX, y, TileLayout::kCenterWidth,
            TileLayout::kTileHeight, TileSize::Wide};
}

constexpr TileGeometryList kLeftGeometry{{
    small(TileLayout::kOuterMargin, TileLayout::kRowY[0], TileLayout::kLeftWidth),
    small(TileLayout::kOuterMargin, TileLayout::kRowY[1], TileLayout::kLeftWidth),
    small(TileLayout::kOuterMargin, TileLayout::kRowY[2], TileLayout::kLeftWidth),
    small(TileLayout::kOuterMargin, TileLayout::kRowY[3], TileLayout::kLeftWidth),
}};

constexpr TileGeometryList kRightGeometry{{
    small(TileLayout::kRightX, TileLayout::kRowY[0], TileLayout::kRightWidth),
    small(TileLayout::kRightX, TileLayout::kRowY[1], TileLayout::kRightWidth),
    small(TileLayout::kRightX, TileLayout::kRowY[2], TileLayout::kRightWidth),
    small(TileLayout::kRightX, TileLayout::kRowY[3], TileLayout::kRightWidth),
}};

constexpr std::array<TileGeometry, 2> kDashWideGeometry{{
    wide(TileLayout::kRowY[0]),
    wide(TileLayout::kRowY[1]),
}};

constexpr TileGeometryList kDashCenterSmallGeometry{{
    small(TileLayout::kCenterX, TileLayout::kRowY[2],
          TileLayout::kCenterSmallWidth),
    small(TileLayout::kCenterX + TileLayout::kCenterSmallWidth +
              TileLayout::kColumnGap,
          TileLayout::kRowY[2], TileLayout::kCenterSmallWidth),
    small(TileLayout::kCenterX, TileLayout::kRowY[3],
          TileLayout::kCenterSmallWidth),
    small(TileLayout::kCenterX + TileLayout::kCenterSmallWidth +
              TileLayout::kColumnGap,
          TileLayout::kRowY[3], TileLayout::kCenterSmallWidth),
}};

constexpr TileGeometryList kTrackCenterGeometry{{
    wide(TileLayout::kRowY[0]),
    wide(TileLayout::kRowY[1]),
    wide(TileLayout::kRowY[2]),
    wide(TileLayout::kRowY[3]),
}};

template <typename Tiles, std::size_t GeometryCount>
void appendGroup(TilePlacementList& output, PageId page,
                 const Tiles& tiles,
                 uint8_t first_slot, TileGroup group,
                 const std::array<TileGeometry, GeometryCount>& geometry) {
    std::array<uint8_t, GeometryCount> visible_slots{};
    std::size_t visible_count = 0U;
    for (std::size_t i = 0U; i < GeometryCount; ++i) {
        const uint8_t slot = static_cast<uint8_t>(first_slot + i);
        if (slot < tiles.size() && tiles[slot].visible) {
            visible_slots[visible_count++] = slot;
        }
    }

    const std::size_t first_geometry = GeometryCount - visible_count;
    for (std::size_t i = 0U; i < visible_count; ++i) {
        output.items[output.count++] = {
            {page, visible_slots[i]}, group, geometry[first_geometry + i]};
    }
}
}  // namespace

TilePlacementList TileEngine::placements(PageId page,
                                         const AppConfig& config) {
    TilePlacementList output;
    if (page == PageId::Settings) return output;
    const auto layout = selectedLayout(config, page);
    const auto tiles = activeTiles(config, page);
    if (layout == DashboardLayout::AnalogStyle) {
        std::array<TileGeometry, 6> rows{};
        for (int i = 0; i < 6; ++i)
            rows[i] = {432, static_cast<int16_t>(36 + 64 * i), 360, 58, TileSize::CompactRow};
        appendGroup(output, page, tiles, 0U, TileGroup::DashRight, rows);
    } else if (layout == DashboardLayout::SideGear) {
        const std::array<TileGeometry, 1> gear{{{8, 8, 126, 318, TileSize::GearHero}}};
        const std::array<TileGeometry, 2> main{{
            {142, 112, 320, 214, TileSize::Hero},
            {470, 112, 322, 214, TileSize::Hero}}};
        std::array<TileGeometry, 5> bottom{};
        for (int i = 0; i < 5; ++i)
            bottom[i] = {static_cast<int16_t>(8 + i * 158), 334, 150, 88, TileSize::CenteredSmall};
        appendGroup(output, page, tiles, 0U, TileGroup::DashLeft, gear);
        appendGroup(output, page, tiles, 1U, TileGroup::DashCenterWide, main);
        appendGroup(output, page, tiles, 3U, TileGroup::DashCenterSmall, bottom);
    } else if (layout == DashboardLayout::StripStyle) {
        const std::array<TileGeometry, 3> left{{
            {8, 158, 158, 84, TileSize::CenteredSmall},
            {8, 248, 158, 84, TileSize::CenteredSmall},
            {8, 338, 158, 84, TileSize::CenteredSmall}}};
        const std::array<TileGeometry, 2> center{{
            {174, 158, 224, 264, TileSize::Hero},
            {406, 158, 224, 264, TileSize::Hero}}};
        const std::array<TileGeometry, 3> right{{
            {638, 158, 154, 84, TileSize::CenteredSmall},
            {638, 248, 154, 84, TileSize::CenteredSmall},
            {638, 338, 154, 84, TileSize::CenteredSmall}}};
        appendGroup(output, page, tiles, 0U, TileGroup::DashLeft, left);
        appendGroup(output, page, tiles, 3U, TileGroup::DashCenterWide, center);
        appendGroup(output, page, tiles, 5U, TileGroup::DashRight, right);
    } else if (layout == DashboardLayout::ClassicDash) {
        appendGroup(output, page, tiles, 0U,
                    TileGroup::DashLeft, kLeftGeometry);
        appendGroup(output, page, tiles, 4U,
                    TileGroup::DashCenterWide, kDashWideGeometry);
        appendGroup(output, page, tiles, 6U,
                    TileGroup::DashCenterSmall, kDashCenterSmallGeometry);
        appendGroup(output, page, tiles, 10U,
                    TileGroup::DashRight, kRightGeometry);
    } else if (layout == DashboardLayout::ClassicTrack) {
        appendGroup(output, page, tiles, 0U,
                    TileGroup::TrackLeft, kLeftGeometry);
        appendGroup(output, page, tiles, 4U,
                    TileGroup::TrackCenter, kTrackCenterGeometry);
        appendGroup(output, page, tiles, 8U,
                    TileGroup::TrackRight, kRightGeometry);
    }
    return output;
}
