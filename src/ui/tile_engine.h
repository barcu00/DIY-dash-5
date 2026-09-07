#pragma once

#include <array>
#include <cstddef>

#include "settings/app_config.h"
#include "ui/tile_layout.h"

struct TilePlacement {
    TileAddress address{};
    TileGroup group = TileGroup::DashLeft;
    TileGeometry geometry{};
};

struct TilePlacementList {
    std::array<TilePlacement, AppConfig::kDashTileCount> items{};
    std::size_t count = 0U;
};

class TileEngine {
public:
    static TilePlacementList placements(PageId page, const AppConfig& config);
};
