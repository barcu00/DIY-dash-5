#pragma once
#include <array>
#include <cstdint>
#include <lvgl.h>
#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"
#include "ui/display_signal_filter.h"
#include "ui/tile_layout.h"
class TileView {
public:
    void create(lv_obj_t* parent, TileAddress address, lv_event_cb_t callback);
    void apply(const TileConfig& config, const TileGeometry& geometry);
    void hide();
    void update(const TileConfig& config, const UnitSettings& units,
                const VehicleState& state, bool warning_active,
                uint32_t now_ms);
    TileAddress address() const;
private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* value_ = nullptr;
    lv_obj_t* unit_ = nullptr;
    TileAddress address_{};
    DisplaySignalFilter display_filter_{};
    std::array<char, 32> last_value_text_{};
    std::array<char, 16> last_unit_text_{};
    uint32_t next_refresh_ms_ = 0U;
    bool refresh_initialized_ = false;
    bool value_text_initialized_ = false;
    bool unit_text_initialized_ = false;
    bool raw_valid_initialized_ = false;
    bool last_raw_valid_ = false;
    bool warning_initialized_ = false;
    bool last_warning_active_ = false;
};
