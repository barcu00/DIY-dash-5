#include "dashboard_layout.h"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include "telemetry/parameter_registry.h"

namespace {
template <typename Config>
auto bank(Config& config, PageId page, DashboardLayout layout) {
    using Tile = decltype(config.dash_tiles.data());
    Tile data = nullptr;
    std::size_t count = 0U;
    if ((page == PageId::Dash || page == PageId::Track) &&
        validDashboardLayout(layout)) {
        count = dashboardLayoutSlotCount(layout);
        const auto default_layout = page == PageId::Dash
            ? DashboardLayout::ClassicDash : DashboardLayout::ClassicTrack;
        if (layout == default_layout) {
            data = page == PageId::Dash ? config.dash_tiles.data()
                                        : config.track_tiles.data();
        } else {
            const auto value = static_cast<std::size_t>(layout);
            const auto index = value - (value > static_cast<std::size_t>(default_layout) ? 1U : 0U);
            auto& alternatives = page == PageId::Dash
                ? config.dash_alternate_tiles : config.track_alternate_tiles;
            data = alternatives[index].data();
        }
    }
    return TileBankView<std::remove_pointer_t<Tile>>{data, count};
}

TileConfig makeTile(ParameterId parameter) {
    TileConfig tile;
    tile.parameter = parameter;
    tile.decimals = parameterDescriptor(parameter).default_decimals;
    tile.temperature_bar = defaultTemperatureBarConfig(parameter);
    return tile;
}
}  // namespace

bool validDashboardLayout(DashboardLayout layout) {
    return static_cast<std::size_t>(layout) < kDashboardLayoutCount;
}

const char* dashboardLayoutName(DashboardLayout layout) {
    constexpr const char* names[] = {"CLASSIC DASH", "CLASSIC TRACK",
        "ANALOG STYLE", "SIDE GEAR", "STRIP STYLE", "MODERN MOTORSPORT"};
    return validDashboardLayout(layout) ? names[static_cast<std::size_t>(layout)]
                                        : names[0];
}

std::size_t dashboardLayoutSlotCount(DashboardLayout layout) {
    constexpr std::size_t counts[] = {14U, 12U, 6U, 8U, 8U, 6U};
    return validDashboardLayout(layout) ? counts[static_cast<std::size_t>(layout)] : 0U;
}

DashboardLayout selectedLayout(const AppConfig& config, PageId page) {
    return page == PageId::Track ? config.track_layout : config.dash_layout;
}

TileBankView<TileConfig> layoutTiles(AppConfig& config, PageId page,
                                   DashboardLayout layout) {
    return bank(config, page, layout);
}
TileBankView<const TileConfig> layoutTiles(const AppConfig& config, PageId page,
                                         DashboardLayout layout) {
    return bank(config, page, layout);
}
TileBankView<TileConfig> activeTiles(AppConfig& config, PageId page) {
    return layoutTiles(config, page, selectedLayout(config, page));
}
TileBankView<const TileConfig> activeTiles(const AppConfig& config, PageId page) {
    return layoutTiles(config, page, selectedLayout(config, page));
}

void initializeAlternateLayouts(AppConfig& config) {
    for (const PageId page : {PageId::Dash, PageId::Track}) {
        for (std::size_t i = 0; i < kDashboardLayoutCount; ++i) {
            const auto layout = static_cast<DashboardLayout>(i);
            if ((page == PageId::Dash && layout == DashboardLayout::ClassicDash) ||
                (page == PageId::Track && layout == DashboardLayout::ClassicTrack)) continue;
            auto tiles = layoutTiles(config, page, layout);
            if (layout == DashboardLayout::ClassicDash) {
                std::copy(config.dash_tiles.begin(), config.dash_tiles.end(), tiles.begin());
            } else if (layout == DashboardLayout::ClassicTrack) {
                std::copy(config.track_tiles.begin(), config.track_tiles.end(), tiles.begin());
            } else {
                constexpr ParameterId analog[] = {ParameterId::Speed,
                    ParameterId::OilPressure, ParameterId::OilTemperature,
                    ParameterId::Clt, ParameterId::Map, ParameterId::Lambda};
                constexpr ParameterId side[] = {ParameterId::Gear, ParameterId::Rpm,
                    ParameterId::Speed, ParameterId::OilPressure,
                    ParameterId::OilTemperature, ParameterId::Clt,
                    ParameterId::Map, ParameterId::Lambda};
                constexpr ParameterId strip[] = {ParameterId::OilPressure,
                    ParameterId::OilTemperature, ParameterId::FuelPressure,
                    ParameterId::Rpm, ParameterId::Speed, ParameterId::Clt,
                    ParameterId::Iat, ParameterId::Lambda};
                const ParameterId* parameters = (layout == DashboardLayout::AnalogStyle || layout == DashboardLayout::ModernMotorsport)
                    ? analog : layout == DashboardLayout::SideGear ? side : strip;
                for (std::size_t slot = 0; slot < tiles.size(); ++slot)
                    tiles[slot] = makeTile(parameters[slot]);
            }
        }
    }
}

void resetPageLayouts(AppConfig& config, PageId page) {
    const AppConfig defaults = AppConfig::defaults();
    if (page == PageId::Dash) {
        config.dash_tiles = defaults.dash_tiles;
        config.dash_alternate_tiles = defaults.dash_alternate_tiles;
        config.dash_layout = defaults.dash_layout;
    } else if (page == PageId::Track) {
        config.track_tiles = defaults.track_tiles;
        config.track_alternate_tiles = defaults.track_alternate_tiles;
        config.track_layout = defaults.track_layout;
    }
}

uint16_t normalizedRpmScale(uint16_t requested) {
    const uint32_t bounded = std::clamp<uint32_t>(requested, 100U, 10000U);
    return static_cast<uint16_t>(((bounded + 50U) / 100U) * 100U);
}

uint16_t rpmScaleFill(float rpm, bool valid, uint16_t maximum) {
    if (!valid || !std::isfinite(rpm) || maximum == 0U) return 0U;
    return static_cast<uint16_t>(std::clamp(rpm / maximum, 0.0f, 1.0f) * 1000.0f);
}
