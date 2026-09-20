#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <lvgl.h>
#include "alarms/tile_warning_engine.h"
#include "board/board_display.h"
#include "can/can_status.h"
#include "ecu/parameter_capabilities.h"
#include "settings/app_config.h"
#include "telemetry/vehicle_state.h"
#include "telemetry/composite_telemetry_view.h"
#include "ui/settings_commit_model.h"
#include "ui/parameter_options.h"
#include "ui/tile_editor_model.h"
#include "ui/shift_light_view.h"
#include "ui/settings_flow_model.h"
#include "ui/tile_view.h"
#include "ui/ui_update_policy.h"
#include "ui/rpm_scale_view.h"
#include "ui/numeric_entry_model.h"
#include "ui/racechrono_settings_model.h"
#include "racechrono/racechrono_session.h"
struct UiRuntimeStatus {
    CanStatus can_status = CanStatus::Waiting;
    bool demo_active = false;
    uint32_t can_bitrate = 0U;
    uint32_t can_timeout_ms = 0U;
    std::size_t decoder_mappings = 0U;
    uint32_t received_frames = 0U;
    uint32_t rejected_frames = 0U;
    RaceChronoConnectionState racechrono_connection =
        RaceChronoConnectionState::Disabled;
    RaceChronoRuntimeStatus racechrono{};
};
class Ui {
public:
    void begin(AppConfig& config, BoardDisplay& board);
    void update(const CompositeTelemetryView& state,
                const RuntimeDiagnostics& diagnostics,
                const UiRuntimeStatus& status, const AppConfig& config,
                TileWarningEngine& warnings);
    void updateShiftLight(const VehicleState& state, uint32_t now_ms,
                          const ShiftLightConfig& config);
    void setDataContext(DataSource source, const CanProfile* profile);
    bool takeConfigCommit(ConfigCommitRequest& request);
    void completeConfigCommit(uint32_t revision, bool success);
    bool takeWarningTest();
    bool takeRaceChronoRestart();
    static void spinDecreaseEvent(lv_event_t* event);
    static void spinIncreaseEvent(lv_event_t* event);
private:
    enum class Page : uint8_t { Dash, Track, Settings };
    static void navEvent(lv_event_t* event);
    static void tileEvent(lv_event_t* event);
    static void editorEvent(lv_event_t* event);
    static void editorTabEvent(lv_event_t* event);
    static void numericOpenEvent(lv_event_t* event);
    static void numericKeyEvent(lv_event_t* event);
    static void pickerEvent(lv_event_t* event);
    static void warningTestEvent(lv_event_t* event);
    static void settingsEvent(lv_event_t* event);
    static void settingsCategoryEvent(lv_event_t* event);
    static void settingsBackEvent(lv_event_t* event);
    static void discardSettingsExitEvent(lv_event_t* event);
    static void warningEvent(lv_event_t* event);
    static void layoutSlotEvent(lv_event_t* event);
    static void layoutPageEvent(lv_event_t* event);
    static void layoutSelectEvent(lv_event_t* event);
    static void settingsResetEvent(lv_event_t* event);
    static void settingsConfirmEvent(lv_event_t* event);
    static void raceChronoEvent(lv_event_t* event);
    static void raceChronoBackEvent(lv_event_t* event);
    void createDataPage(Page page, const AppConfig& config);
    void showSettings(SettingsCategory category);
    lv_obj_t* createSettingsPanel(const char* title);
    void createSettingsHome(lv_obj_t* panel);
    void createDisplaySettings(lv_obj_t* panel);
    void createDataCanSettings(lv_obj_t* panel);
    void createRaceChronoSettings(lv_obj_t* panel);
    void refreshRaceChronoSettings();
    void createShiftSettings(lv_obj_t* panel);
    void refreshShiftControls();
    void createUnitSettings(lv_obj_t* panel);
    void createLayoutSettings(lv_obj_t* panel);
    void createSystemSettings(lv_obj_t* panel);
    void clearSettingsWidgets();
    void createNavigation(lv_obj_t* parent, Page active);
    void applyLayout(Page page, const AppConfig& config);
    void load(Page page);
    void prepareDataPage(Page page);
    void openEditor(TileAddress address);
    void closeEditor();
    void saveEditor();
    void syncEditorDraftFromControls();
    void loadEditorControlsFromDraft();
    void refreshEditorParameterControls(ParameterId parameter);
    void loadEditorTemperatureControls(const TemperatureBarConfig& config);
    void showEditorTab(uint8_t tab);
    void refreshEditorPreview();
    void bindNumeric(lv_obj_t* widget,const char* title,float minimum,float maximum,
                     uint8_t decimals,const char* unit,bool slider=false);
    void openNumericEntry(std::size_t index);
    void refreshNumericEntry();
    void refreshNumericFields();
    void closeAuxiliary();
    void openParameterPicker();
    void renderParameterPicker();
    bool stageSettings(AppConfig candidate, bool reconfigure_runtime);
    bool queueSettingsOnExit();
    const AppConfig& settingsConfig() const;
    void beginSettingsSession();
    void requestSettingsExit(Page destination);
    void requestSettingsExit(SettingsCategory destination);
    void finishSettingsExit();
    void showSettingsSaveFailure();
    void clearSettingsSaveFailure();
    void discardFailedSettingsExit();
    void setSettingsCommitBlocked(bool blocked);
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
    lv_obj_t* warning_sound_ = nullptr;
    std::array<TileView, AppConfig::kDashTileCount> dash_tiles_{};
    std::array<TileView, AppConfig::kLayoutTileCapacity> track_tiles_{};
    RpmScaleView dash_rpm_{};
    RpmScaleView track_rpm_{};
    ShiftLightView dash_shift_{};
    ShiftLightView track_shift_{};
    UiUpdatePolicy update_policy_{};
    VehicleState latest_state_{};
    RaceChronoTelemetry latest_racechrono_{};
    UiRuntimeStatus latest_ui_status_{};
    uint32_t latest_state_ms_ = 0U;
    std::array<bool, AppConfig::kDashTileCount> dash_highlights_{};
    std::array<bool, AppConfig::kLayoutTileCapacity> track_highlights_{};
    ParameterCapabilities capabilities_{};
    DataSource active_source_ = DataSource::Demo;
    const CanProfile* active_profile_ = nullptr;
    SettingsFlowModel settings_flow_{};
    AppConfig* config_ = nullptr;
    AppConfig settings_draft_ = AppConfig::defaults();
    bool settings_draft_active_ = false;
    bool pending_settings_exit_ = false;
    bool pending_settings_exit_to_page_ = false;
    Page pending_settings_page_ = Page::Dash;
    SettingsCategory pending_settings_category_ = SettingsCategory::Home;
    BoardDisplay* board_ = nullptr;
    SettingsCommitModel commit_model_{};
    TileEditorModel editor_{};
    const char* settings_feedback_ = "";
    lv_obj_t* settings_message_ = nullptr;
    lv_obj_t* settings_back_ = nullptr;
    lv_obj_t* settings_commit_blocker_ = nullptr;
    lv_obj_t* settings_discard_exit_ = nullptr;
    bool settings_save_failed_ = false;
    lv_obj_t* commit_toast_ = nullptr;
    uint32_t commit_toast_until_ms_ = 0U;
    lv_obj_t* brightness_slider_ = nullptr;
    lv_obj_t* brightness_value_ = nullptr;
    lv_obj_t* source_dropdown_ = nullptr;
    lv_obj_t* profile_dropdown_ = nullptr;
    lv_obj_t* profile_recommendation_ = nullptr;
    lv_obj_t* bitrate_dropdown_ = nullptr;
    lv_obj_t* can_timeout_ = nullptr;
    RaceChronoSettingsModel racechrono_settings_{};
    bool racechrono_channels_tab_ = false;
    bool racechrono_restart_requested_ = false;
    lv_obj_t* racechrono_enabled_ = nullptr;
    lv_obj_t* racechrono_connection_ = nullptr;
    lv_obj_t* racechrono_last_data_ = nullptr;
    lv_obj_t* racechrono_packets_ = nullptr;
    lv_obj_t* racechrono_active_ = nullptr;
    lv_obj_t* racechrono_satellites_ = nullptr;
    lv_obj_t* racechrono_accuracy_ = nullptr;
    lv_obj_t* racechrono_signal_ = nullptr;
    std::array<lv_obj_t*, RaceChronoSettingsModel::kRowsPerPage>
        racechrono_channel_names_{};
    std::array<lv_obj_t*, RaceChronoSettingsModel::kRowsPerPage>
        racechrono_channel_states_{};
    std::array<lv_obj_t*, RaceChronoSettingsModel::kRowsPerPage>
        racechrono_channel_values_{};
    lv_obj_t* racechrono_previous_page_ = nullptr;
    lv_obj_t* racechrono_next_page_ = nullptr;
    lv_obj_t* racechrono_page_number_ = nullptr;
    lv_obj_t* shift_start_ = nullptr;
    lv_obj_t* shift_red_ = nullptr;
    lv_obj_t* shift_flash_ = nullptr;
    lv_obj_t* shift_max_ = nullptr;
    lv_obj_t* shift_flash_enabled_ = nullptr;
    lv_obj_t* shift_start_value_ = nullptr;
    lv_obj_t* shift_red_value_ = nullptr;
    lv_obj_t* shift_flash_value_ = nullptr;
    lv_obj_t* shift_max_value_ = nullptr;
    std::array<lv_obj_t*,36> rpm_preview_blocks_{};
    lv_obj_t* temp_unit_ = nullptr;
    lv_obj_t* pressure_unit_ = nullptr;
    lv_obj_t* speed_unit_ = nullptr;
    lv_obj_t* mixture_unit_ = nullptr;
    lv_obj_t* dash_layout_dropdown_ = nullptr;
    lv_obj_t* track_layout_dropdown_ = nullptr;
    lv_obj_t* rpm_scale_slider_ = nullptr;
    lv_obj_t* rpm_scale_value_ = nullptr;
    std::array<lv_obj_t*, SettingsFlowModel::kSlotsPerPage> layout_labels_{};
    std::array<std::size_t, SettingsFlowModel::kSlotsPerPage> layout_slots_{};
    lv_obj_t* reset_overlay_ = nullptr;
    lv_obj_t* reset_cancel_ = nullptr;
    lv_obj_t* reset_confirm_ = nullptr;
    bool reset_commit_pending_ = false;
    SettingsResetTarget reset_commit_target_ = SettingsResetTarget::DashLayout;
    Page editor_return_page_ = Page::Dash;
    SettingsCategory editor_return_category_ = SettingsCategory::Home;
    lv_obj_t* editor_screen_ = nullptr;
    lv_obj_t* editor_data_panel_ = nullptr;
    lv_obj_t* editor_temperature_panel_ = nullptr;
    lv_obj_t* editor_summary_ = nullptr;
    lv_obj_t* editor_reset_label_ = nullptr;
    lv_obj_t* editor_context_ = nullptr;
    std::array<lv_obj_t*,6> editor_decimal_buttons_{};
    std::array<lv_obj_t*,3> editor_temperature_zones_{};
    lv_obj_t* editor_temperature_marker_=nullptr;
    lv_obj_t* editor_temperature_value_=nullptr;
    lv_obj_t* editor_preview_value_ = nullptr;
    lv_obj_t* editor_preview_name_ = nullptr;
    lv_obj_t* editor_preview_unit_ = nullptr;
    lv_obj_t* editor_preview_bar_ = nullptr;
    std::array<lv_obj_t*,3> editor_tabs_{};
    uint8_t editor_tab_ = 0;
    bool editor_controls_valid_ = true;
    lv_obj_t* auxiliary_screen_ = nullptr;
    lv_obj_t* auxiliary_return_ = nullptr;
    lv_obj_t* numeric_text_ = nullptr;
    lv_obj_t* numeric_apply_ = nullptr;
    NumericEntryModel numeric_{};
    struct NumericBinding {
        lv_obj_t* widget=nullptr;const char* title="";float minimum=0,maximum=0;
        uint8_t decimals=0;const char* unit="";bool slider=false;
    };
    std::array<NumericBinding,16> numeric_bindings_{};
    std::size_t numeric_binding_count_=0;
    NumericBinding active_numeric_{};
    uint8_t picker_category_=0;
    std::size_t picker_page_=0;
    ParameterId picker_selected_=ParameterId::Rpm;
    bool warning_test_requested_=false;
    lv_obj_t* warning_test_screen_=nullptr;
    lv_obj_t* editor_parameter_value_ = nullptr;
    lv_obj_t* editor_visible_ = nullptr;
    lv_obj_t* editor_decimals_label_ = nullptr;
    lv_obj_t* editor_decimals_ = nullptr;
    lv_obj_t* editor_numeric_panel_ = nullptr;
    lv_obj_t* editor_flag_panel_ = nullptr;
    lv_obj_t* editor_flag_color_ = nullptr;
    lv_obj_t* editor_warning_ = nullptr;
    lv_obj_t* editor_temperature_bar_ = nullptr;
    lv_obj_t* editor_temperature_minimum_ = nullptr;
    lv_obj_t* editor_temperature_ready_ = nullptr;
    lv_obj_t* editor_temperature_red_ = nullptr;
    lv_obj_t* editor_temperature_maximum_ = nullptr;
    lv_obj_t* editor_direction_ = nullptr;
    lv_obj_t* editor_threshold_ = nullptr;
    lv_obj_t* editor_hysteresis_ = nullptr;
    lv_obj_t* editor_delay_ = nullptr;
    lv_obj_t* editor_message_ = nullptr;
    lv_obj_t* editor_cancel_ = nullptr;
    lv_obj_t* editor_save_ = nullptr;
    bool editor_commit_pending_ = false;
    lv_obj_t* warning_panel_ = nullptr;
    lv_obj_t* warning_text_ = nullptr;
    TileWarningEngine* warning_engine_ = nullptr;
    TileAddress warning_address_{};
};
