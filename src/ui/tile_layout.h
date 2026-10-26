#pragma once

#include <cstdint>

#include "settings/app_config.h"

struct TileGeometry {
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;
    TileSize size = TileSize::Small;
    // Chamfered analog-style panel: 0 none, 1 top-left/bottom-right cuts
    // (left column), 2 top-right/bottom-left cuts (right column).
    uint8_t chamfer = 0U;
};

namespace TileLayout {
constexpr int16_t kScreenWidth = 800;
constexpr int16_t kScreenHeight = 480;
constexpr int16_t kOuterMargin = 8;
constexpr int16_t kColumnGap = 8;
constexpr int16_t kLeftWidth = 176;
constexpr int16_t kCenterX = 192;
constexpr int16_t kCenterWidth = 416;
constexpr int16_t kCenterSmallWidth = 204;
constexpr int16_t kRightX = 616;
constexpr int16_t kRightWidth = 176;
// Rows sit below the tachometer bar and its scale (bar 8..32, scale to 54).
constexpr int16_t kTileHeight = 86;
constexpr int16_t kNavigationY = 430;
constexpr int16_t kNavigationHeight = 50;
constexpr int16_t kShiftY = 8;
constexpr int16_t kShiftHeight = 24;
constexpr int16_t kContentY = 58;
constexpr int16_t kRowY[4] = {58, 150, 242, 334};
}  // namespace TileLayout
