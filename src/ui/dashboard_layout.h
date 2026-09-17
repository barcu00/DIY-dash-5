#pragma once

#include <cstddef>
#include <cstdint>
#include "settings/app_config.h"

template <typename Tile>
struct TileBankView {
    Tile* data = nullptr;
    std::size_t count = 0U;
    Tile& operator[](std::size_t index) const { return data[index]; }
    std::size_t size() const { return count; }
    Tile* begin() const { return data; }
    Tile* end() const { return data ? data + count : nullptr; }
};

constexpr std::size_t kDashboardLayoutCount = 5U;
bool validDashboardLayout(DashboardLayout layout);
const char* dashboardLayoutName(DashboardLayout layout);
std::size_t dashboardLayoutSlotCount(DashboardLayout layout);
DashboardLayout selectedLayout(const AppConfig& config, PageId page);
TileBankView<TileConfig> layoutTiles(AppConfig& config, PageId page,
                                   DashboardLayout layout);
TileBankView<const TileConfig> layoutTiles(const AppConfig& config, PageId page,
                                         DashboardLayout layout);
TileBankView<TileConfig> activeTiles(AppConfig& config, PageId page);
TileBankView<const TileConfig> activeTiles(const AppConfig& config, PageId page);
void initializeAlternateLayouts(AppConfig& config);
void resetPageLayouts(AppConfig& config, PageId page);
uint16_t normalizedRpmScale(uint16_t requested);
uint16_t rpmScaleFill(float rpm, bool valid, uint16_t maximum);
