#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <lvgl.h>
#include "alarms/tile_warning_engine.h"
#include "board/board_display.h"
#include "can/can_status.h"
#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"
#include "ui/shift_light_view.h"
#include "ui/tile_view.h"
struct UiRuntimeStatus {
    CanStatus can_status = CanStatus::Waiting;
    bool demo_active = false;
    uint32_t can_bitrate = 0U;
    uint32_t can_timeout_ms = 0U;
    std::size_t decoder_mappings = 0U;
    uint32_t received_frames = 0U;
    uint32_t rejected_frames = 0U;
};
class Ui {
public:
    void begin(const AppConfig& config);
    void update(const VehicleState& state, const RuntimeDiagnostics& diagnostics,
                const UiRuntimeStatus& status, const AppConfig& config,
                const TileWarningEngine& warnings);
private:
    enum class Page : uint8_t { Dash, Track, Settings };
    static void navEvent(lv_event_t* event);
    static void tileEvent(lv_event_t* event);
    void createDataPage(Page page, const AppConfig& config);
    void createSettings();
    void createNavigation(lv_obj_t* parent, Page active);
    void applyLayout(Page page, const AppConfig& config);
    void load(Page page);
    static void styleScreen(lv_obj_t* screen);
    static Ui* instance_;
    Page current_page_ = Page::Dash;
    lv_obj_t* dash_ = nullptr;
    lv_obj_t* track_ = nullptr;
    lv_obj_t* settings_ = nullptr;
    lv_obj_t* settings_status_ = nullptr;
    std::array<TileView, AppConfig::kDashTileCount> dash_tiles_{};
    std::array<TileView, AppConfig::kTrackTileCount> track_tiles_{};
    ShiftLightView dash_shift_{};
    ShiftLightView track_shift_{};
};
