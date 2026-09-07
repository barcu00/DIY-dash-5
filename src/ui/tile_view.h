#pragma once
#include <lvgl.h>
#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"
#include "ui/tile_layout.h"
class TileView {
public:
    void create(lv_obj_t* parent, TileAddress address, lv_event_cb_t callback);
    void apply(const TileConfig& config, const TileGeometry& geometry);
    void hide();
    void update(const TileConfig& config, const UnitSettings& units,
                const VehicleState& state, bool warning_active);
    TileAddress address() const;
private:
    lv_obj_t* root_ = nullptr;
    lv_obj_t* title_ = nullptr;
    lv_obj_t* value_ = nullptr;
    lv_obj_t* unit_ = nullptr;
    TileAddress address_{};
};
