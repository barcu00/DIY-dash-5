#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "telemetry/vehicle_state.h"
#include "ui/unit_presenter.h"

enum class PageId : uint8_t {
    Dash,
    Track,
    Settings,
};

enum class TileSize : uint8_t {
    Small,
    Wide,
};

enum class WarningDirection : uint8_t {
    Above,
    Below,
};

enum class TileGroup : uint8_t {
    DashLeft,
    DashCenterWide,
    DashCenterSmall,
    DashRight,
    TrackLeft,
    TrackCenter,
    TrackRight,
};

struct TileAddress {
    PageId page = PageId::Dash;
    uint8_t slot = 0U;
};

struct ShiftLightConfig {
    uint16_t start_rpm = 5500U;
    uint16_t red_rpm = 7000U;
    uint16_t flash_rpm = 7500U;
    uint16_t max_rpm = 8000U;
    bool flash_enabled = true;
};

struct CanSettings {
    std::array<char, 32> profile_id{};
    uint32_t bitrate = 500000U;
    uint32_t timeout_ms = 500U;
};

struct TileWarningConfig {
    bool enabled = false;
    WarningDirection direction = WarningDirection::Above;
    float threshold_native = 0.0f;
    float hysteresis_native = 0.0f;
    uint16_t delay_ms = 0U;
};

struct TileConfig {
    ParameterId parameter = ParameterId::Rpm;
    bool visible = true;
    uint8_t decimals = 0U;
    TileWarningConfig warning{};
};

struct ValidationResult {
    bool valid = true;
    bool shift_order_valid = true;
};

struct AppConfig {
    static constexpr uint32_t kSchemaVersion = 2U;
    static constexpr std::size_t kDashTileCount = 14U;
    static constexpr std::size_t kTrackTileCount = 12U;

    uint32_t schema_version = kSchemaVersion;
    DataSource data_source = DataSource::Demo;
    uint8_t brightness_percent = 100U;
    CanSettings can{};
    ShiftLightConfig shift{};
    UnitSettings units{};
    std::array<TileConfig, kDashTileCount> dash_tiles{};
    std::array<TileConfig, kTrackTileCount> track_tiles{};

    static AppConfig defaults();
    ValidationResult validate();
};
