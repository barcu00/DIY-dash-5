#pragma once

#include <cstdint>

#include "settings/app_config.h"

struct TileGeometry {
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;
    TileSize size = TileSize::Small;
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
constexpr int16_t kTileHeight = 90;
constexpr int16_t kNavigationY = 430;
constexpr int16_t kNavigationHeight = 50;
constexpr int16_t kShiftY = 8;
constexpr int16_t kShiftHeight = 16;
constexpr int16_t kRowY[4] = {36, 134, 232, 332};
}  // namespace TileLayout
