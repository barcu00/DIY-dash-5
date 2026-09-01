#pragma once

#include <array>
#include <cstdint>
#include <lvgl.h>

#include "board/board_display.h"
#include "settings/app_config.h"
#include "settings/nvs_config_store.h"
#include "telemetry/data_source.h"
#include "telemetry/parameter_registry.h"
#include "telemetry/vehicle_state.h"
#include "ui/gesture_navigation.h"

struct TelemetryRuntimeStatus {
    DataSource source = DataSource::Mock;
    bool can_driver_running = false;
    bool can_online = false;
    uint32_t can_bitrate = 0;
    uint32_t received_frames = 0;
    uint32_t invalid_frames = 0;
};

class Ui {
public:
    void begin(AppConfig& config, ParameterRegistry& registry, NvsConfigStore& config_store);
    void update(const VehicleState& state, const RuntimeDiagnostics& diag,
                const TelemetryRuntimeStatus& telemetry);

private:
    struct TileView {
        lv_obj_t* container = nullptr;
        lv_obj_t* title = nullptr;
        lv_obj_t* value = nullptr;
        lv_obj_t* icon = nullptr;
        std::array<lv_color_t, 16U * 16U> icon_buffer{};
        uint8_t config_index = 0U;
        bool track = false;
    };

    static void gestureEvent(lv_event_t* event);
    static void tileEvent(lv_event_t* event);

    static void createShiftBar(lv_obj_t* parent, lv_obj_t** segments);
    void createDash();
    void createDiag();
    void createTrack();
    void createSettings();
    void createConfigurableTiles(lv_obj_t* parent,
                                 std::array<TileView, AppConfig::kTileCount>& views,
                                 bool track);
    void refreshTileLayout(std::array<TileView, AppConfig::kTileCount>& views,
                           const std::array<TileConfig, AppConfig::kTileCount>& config);
    void refreshTileValues(std::array<TileView, AppConfig::kTileCount>& views,
                           const std::array<TileConfig, AppConfig::kTileCount>& config);
    void renderTileIcon(TileView& view, const TileConfig& config);
    void refreshAllTiles();
    void openTileEditor(TileView& view);
    void closeTileEditor();
    void saveConfig();

    void load(UiPage page);
    static void updateShiftBar(lv_obj_t** segments, uint16_t rpm);

    static Ui* instance_;

    AppConfig* config_ = nullptr;
    ParameterRegistry* registry_ = nullptr;
    NvsConfigStore* config_store_ = nullptr;

    UiPage current_page_ = UiPage::Dash;
    lv_obj_t* dash_ = nullptr;
    lv_obj_t* diag_ = nullptr;
    lv_obj_t* track_ = nullptr;
    lv_obj_t* settings_ = nullptr;

    lv_obj_t* dash_rpm_ = nullptr;
    lv_obj_t* dash_gear_ = nullptr;
    lv_obj_t* dash_source_ = nullptr;
    lv_obj_t* dash_shift_[12]{};
    std::array<TileView, AppConfig::kTileCount> dash_tiles_{};

    lv_obj_t* diag_status_ = nullptr;
    lv_obj_t* diag_memory_ = nullptr;
    lv_obj_t* diag_runtime_ = nullptr;
    lv_obj_t* diag_source_ = nullptr;
    lv_obj_t* diag_can_box_ = nullptr;
    lv_obj_t* diag_can_state_ = nullptr;
    lv_obj_t* diag_can_stats_ = nullptr;

    lv_obj_t* track_rpm_ = nullptr;
    lv_obj_t* track_gear_ = nullptr;
    lv_obj_t* track_shift_[12]{};
    std::array<TileView, AppConfig::kTileCount> track_tiles_{};

    lv_obj_t* tile_editor_overlay_ = nullptr;
    TileView* editing_tile_ = nullptr;
};
