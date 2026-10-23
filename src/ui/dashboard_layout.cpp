#include "dashboard_layout.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <new>
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
        const auto value = static_cast<std::size_t>(layout);
        const auto default_layout = page == PageId::Dash
            ? DashboardLayout::ClassicDash : DashboardLayout::ClassicTrack;
        if (value >= static_cast<std::size_t>(DashboardLayout::OemSport)) {
            auto& extra = page == PageId::Dash ? config.dash_extra_tiles
                                               : config.track_extra_tiles;
            data = extra[value - static_cast<std::size_t>(DashboardLayout::OemSport)].data();
        } else if (layout == default_layout) {
            data = page == PageId::Dash ? config.dash_tiles.data()
                                        : config.track_tiles.data();
        } else {
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

bool layoutHasTachBar(DashboardLayout layout) {
    switch (layout) {
        case DashboardLayout::ClassicDash:
        case DashboardLayout::ClassicTrack:
        case DashboardLayout::OemSport:
        case DashboardLayout::TrackFocus:
        case DashboardLayout::Endurance:
            return true;
        default:
            return false;
    }
}

bool layoutIsDial(DashboardLayout layout) {
    return layout == DashboardLayout::AnalogStyle ||
           layout == DashboardLayout::ModernMotorsport ||
           layout == DashboardLayout::AnalogClassic ||
           layout == DashboardLayout::AnalogTwin;
}

const char* dashboardLayoutName(DashboardLayout layout) {
    constexpr const char* names[] = {"CLASSIC DASH", "CLASSIC TRACK",
        "ANALOG STYLE", "SIDE GEAR", "STRIP STYLE", "MODERN MOTORSPORT",
        "OEM SPORT", "TRACK FOCUS", "ENDURANCE", "ANALOG CLASSIC",
        "ANALOG TWIN"};
    static_assert(sizeof(names) / sizeof(names[0]) == kDashboardLayoutCount,
                  "every layout needs a name");
    return validDashboardLayout(layout) ? names[static_cast<std::size_t>(layout)]
                                        : names[0];
}

std::size_t dashboardLayoutSlotCount(DashboardLayout layout) {
    constexpr std::size_t counts[] = {14U, 12U, 7U, 8U, 8U, 6U,
                                      10U, 11U, 9U, 8U, 5U};
    static_assert(sizeof(counts) / sizeof(counts[0]) == kDashboardLayoutCount,
                  "every layout needs a slot count");
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
        for (std::size_t i = 0; i < static_cast<std::size_t>(DashboardLayout::OemSport); ++i) {
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
                    ParameterId::Clt, ParameterId::Map, ParameterId::Lambda, ParameterId::Rpm};
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
    initializeExtraLayouts(config);
}

void initializeExtraLayouts(AppConfig& config) {
    using P = ParameterId;
    // Defaults per layout and page, in slot order (see TileEngine geometry).
    struct Preset { DashboardLayout layout; PageId page; P slots[12]; uint16_t hidden_mask; };
    static const Preset presets[] = {
        {DashboardLayout::OemSport, PageId::Dash,
         {P::Clt, P::Speed, P::OilTemperature, P::BatteryVoltage, P::Gear, P::Map,
          P::OilPressure, P::Odometer, P::TripA, P::TripB}, 0U},
        {DashboardLayout::OemSport, PageId::Track,
         {P::Clt, P::RcLapTime, P::OilTemperature, P::RcBestLapTime, P::Gear,
          P::RcPreviousLapTime, P::OilPressure, P::RcLapNumber, P::RcDeltaLapTime,
          P::RcGpsSpeed}, 0U},
        {DashboardLayout::TrackFocus, PageId::Dash,
         {P::OilPressure, P::OilTemperature, P::Clt, P::Speed, P::RcDeltaLapTime,
          P::RcLapTime, P::Gear, P::Rpm, P::RcSectorTime, P::RcLapNumber,
          P::RcBestLapTime}, 0U},
        {DashboardLayout::TrackFocus, PageId::Track,
         {P::OilPressure, P::OilTemperature, P::Clt, P::RcLapNumber, P::RcBestLapTime,
          P::RcPreviousLapTime, P::RcDeltaLapTime, P::RcLapTime, P::Gear, P::Rpm,
          P::Speed}, 0U},
        {DashboardLayout::Endurance, PageId::Dash,
         {P::OilTemperature, P::OilPressure, P::Clt, P::BatteryVoltage, P::FuelPressure,
          P::Iat, P::Gear, P::Speed, P::Rpm}, 0U},
        {DashboardLayout::Endurance, PageId::Track,
         {P::OilTemperature, P::OilPressure, P::Clt, P::BatteryVoltage,
          P::RcPreviousLapTime, P::RcBestLapTime, P::RcDeltaLapTime, P::RcLapTime,
          P::RcLapNumber}, 0U},
        {DashboardLayout::AnalogClassic, PageId::Dash,
         {P::OilPressure, P::OilTemperature, P::Clt, P::Lambda, P::Map,
          P::BatteryVoltage, P::Speed, P::Gear}, 0U},
        {DashboardLayout::AnalogClassic, PageId::Track,
         {P::OilPressure, P::OilTemperature, P::Clt, P::RcDeltaLapTime, P::RcLapTime,
          P::RcBestLapTime, P::Speed, P::Gear}, 0U},
        {DashboardLayout::AnalogTwin, PageId::Dash,
         {P::Gear, P::OilPressure, P::Clt, P::Speed, P::BatteryVoltage}, 0U},
        {DashboardLayout::AnalogTwin, PageId::Track,
         {P::Gear, P::OilPressure, P::Clt, P::RcLapTime, P::RcDeltaLapTime}, 0U},
    };
    for (const Preset& preset : presets) {
        auto tiles = layoutTiles(config, preset.page, preset.layout);
        for (std::size_t slot = 0; slot < tiles.size(); ++slot) {
            tiles[slot] = makeTile(preset.slots[slot]);
            tiles[slot].visible = (preset.hidden_mask & (1U << slot)) == 0U;
            // Lap data reads best as 1:42.38; race time as 2:31:08.
            if (preset.slots[slot] == P::RcTotalRaceTime ||
                preset.slots[slot] == P::RcGpsSpeed) tiles[slot].decimals = 0U;
            else if (parameterDescriptor(preset.slots[slot]).native_unit == NativeUnit::Seconds)
                tiles[slot].decimals = 2U;
        }
    }
}

void resetPageLayouts(AppConfig& config, PageId page) {
    // ~12 KB: keep the defaults off the LVGL/loop stack.
    std::unique_ptr<AppConfig> defaults(new (std::nothrow) AppConfig(AppConfig::defaults()));
    if (!defaults) return;
    if (page == PageId::Dash) {
        config.dash_tiles = defaults->dash_tiles;
        config.dash_alternate_tiles = defaults->dash_alternate_tiles;
        config.dash_extra_tiles = defaults->dash_extra_tiles;
        config.dash_layout = defaults->dash_layout;
    } else if (page == PageId::Track) {
        config.track_tiles = defaults->track_tiles;
        config.track_alternate_tiles = defaults->track_alternate_tiles;
        config.track_extra_tiles = defaults->track_extra_tiles;
        config.track_layout = defaults->track_layout;
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
