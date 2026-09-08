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
#include "ui/settings_commit_model.h"
#include "ui/tile_editor_model.h"
#include "ui/shift_light_view.h"
#include "ui/settings_flow_model.h"
#include "ui/tile_view.h"
#include "ui/ui_update_policy.h"
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
    void begin(AppConfig& config, BoardDisplay& board);
    void update(const VehicleState& state, const RuntimeDiagnostics& diagnostics,
                const UiRuntimeStatus& status, const AppConfig& config,
                TileWarningEngine& warnings);
    void updateShiftLight(const VehicleState& state, uint32_t now_ms,
                          const ShiftLightConfig& config);
    bool takeConfigCommit(ConfigCommitRequest& request);
    void completeConfigCommit(uint32_t revision, bool success);
    static void spinDecreaseEvent(lv_event_t* event);
    static void spinIncreaseEvent(lv_event_t* event);
private:
    enum class Page : uint8_t { Dash, Track, Settings };
    static void navEvent(lv_event_t* event);
    static void tileEvent(lv_event_t* event);
    static void editorEvent(lv_event_t* event);
    static void settingsEvent(lv_event_t* event);
    static void settingsCategoryEvent(lv_event_t* event);
    static void settingsBackEvent(lv_event_t* event);
    static void warningEvent(lv_event_t* event);
    static void layoutSlotEvent(lv_event_t* event);
    static void layoutPageEvent(lv_event_t* event);
    static void layoutSelectEvent(lv_event_t* event);
    static void settingsResetEvent(lv_event_t* event);
    static void settingsConfirmEvent(lv_event_t* event);
    void createDataPage(Page page, const AppConfig& config);
    void showSettings(SettingsCategory category);
    lv_obj_t* createSettingsPanel(const char* title);
    void createSettingsHome(lv_obj_t* panel);
    void createDisplaySettings(lv_obj_t* panel);
    void createDataCanSettings(lv_obj_t* panel);
    void createShiftSettings(lv_obj_t* panel);
    void refreshShiftControls();
    void createUnitSettings(lv_obj_t* panel);
    void createLayoutSettings(lv_obj_t* panel);
    void createSystemSettings(lv_obj_t* panel);
    void clearSettingsWidgets();
    void createNavigation(lv_obj_t* parent, Page active);
    void applyLayout(Page page, const AppConfig& config);
    void load(Page page);
    void openEditor(TileAddress address);
    void closeEditor();
    void saveEditor();
    bool stageSettings(AppConfig candidate, bool reconfigure_runtime);
    void queueSettingsOnExit();
    void showCommitFeedback(const char* message);
    void showSettingsMessage(const char* message);
    void openResetConfirmation(SettingsResetTarget target);
    void closeResetConfirmation();
    void confirmReset();
    void updateWarningModal(TileWarningEngine& warnings);
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
    UiUpdatePolicy update_policy_{};
    SettingsFlowModel settings_flow_{};
    AppConfig* config_ = nullptr;
    BoardDisplay* board_ = nullptr;
    SettingsCommitModel commit_model_{};
    TileEditorModel editor_{};
    const char* settings_feedback_ = "";
    lv_obj_t* settings_message_ = nullptr;
    lv_obj_t* commit_toast_ = nullptr;
    uint32_t commit_toast_until_ms_ = 0U;
    lv_obj_t* brightness_slider_ = nullptr;
    lv_obj_t* brightness_value_ = nullptr;
    lv_obj_t* source_dropdown_ = nullptr;
    lv_obj_t* profile_dropdown_ = nullptr;
    lv_obj_t* profile_recommendation_ = nullptr;
    lv_obj_t* bitrate_dropdown_ = nullptr;
    lv_obj_t* can_timeout_ = nullptr;
    lv_obj_t* shift_start_ = nullptr;
    lv_obj_t* shift_red_ = nullptr;
    lv_obj_t* shift_flash_ = nullptr;
    lv_obj_t* shift_max_ = nullptr;
    lv_obj_t* shift_start_value_ = nullptr;
    lv_obj_t* shift_red_value_ = nullptr;
    lv_obj_t* shift_flash_value_ = nullptr;
    lv_obj_t* shift_max_value_ = nullptr;
    lv_obj_t* shift_flash_enabled_ = nullptr;
    lv_obj_t* temp_unit_ = nullptr;
    lv_obj_t* pressure_unit_ = nullptr;
    lv_obj_t* speed_unit_ = nullptr;
    lv_obj_t* mixture_unit_ = nullptr;
    std::array<lv_obj_t*, SettingsFlowModel::kSlotsPerPage> layout_labels_{};
    std::array<std::size_t, SettingsFlowModel::kSlotsPerPage> layout_slots_{};
    lv_obj_t* reset_overlay_ = nullptr;
    lv_obj_t* editor_overlay_ = nullptr;
    lv_obj_t* editor_parameter_ = nullptr;
    lv_obj_t* editor_visible_ = nullptr;
    lv_obj_t* editor_decimals_ = nullptr;
    lv_obj_t* editor_warning_ = nullptr;
    lv_obj_t* editor_direction_ = nullptr;
    lv_obj_t* editor_threshold_ = nullptr;
    lv_obj_t* editor_hysteresis_ = nullptr;
    lv_obj_t* editor_delay_ = nullptr;
    lv_obj_t* editor_message_ = nullptr;
    lv_obj_t* warning_panel_ = nullptr;
    lv_obj_t* warning_text_ = nullptr;
    TileWarningEngine* warning_engine_ = nullptr;
    TileAddress warning_address_{};
};
