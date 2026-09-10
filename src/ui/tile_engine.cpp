#include "tile_engine.h"

#include <array>

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

template <std::size_t ConfigCount, std::size_t GeometryCount>
void appendGroup(TilePlacementList& output, PageId page,
                 const std::array<TileConfig, ConfigCount>& tiles,
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
    if (page == PageId::Dash) {
        appendGroup(output, page, config.dash_tiles, 0U,
                    TileGroup::DashLeft, kLeftGeometry);
        appendGroup(output, page, config.dash_tiles, 4U,
                    TileGroup::DashCenterWide, kDashWideGeometry);
        appendGroup(output, page, config.dash_tiles, 6U,
                    TileGroup::DashCenterSmall, kDashCenterSmallGeometry);
        appendGroup(output, page, config.dash_tiles, 10U,
                    TileGroup::DashRight, kRightGeometry);
    } else if (page == PageId::Track) {
        appendGroup(output, page, config.track_tiles, 0U,
                    TileGroup::TrackLeft, kLeftGeometry);
        appendGroup(output, page, config.track_tiles, 4U,
                    TileGroup::TrackCenter, kTrackCenterGeometry);
        appendGroup(output, page, config.track_tiles, 8U,
                    TileGroup::TrackRight, kRightGeometry);
    }
    return output;
}
