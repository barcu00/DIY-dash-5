#pragma once

#include <cstddef>
#include <cstdint>

#include <lvgl.h>

#include "alarms/alarm_manager.h"
#include "board/board_display.h"
#include "can/can_status.h"
#include "telemetry/vehicle_state.h"

struct UiRuntimeStatus {
    CanStatus can_status = CanStatus::Waiting;
    AlarmSummary alarms{};
    bool demo_active = false;
    uint32_t can_bitrate = 0U;
    uint32_t can_timeout_ms = 0U;
    std::size_t decoder_mappings = 0U;
    uint32_t received_frames = 0U;
    uint32_t rejected_frames = 0U;
};

class Ui {
public:
    void begin();
    void update(const VehicleState& state, const RuntimeDiagnostics& diag,
                const UiRuntimeStatus& status);

private:
    enum class Page : uint8_t { Dash, Track, Diag, Settings };

    static void navEvent(lv_event_t* event);
    static lv_obj_t* createValueTile(lv_obj_t* parent, const char* title, int x, int y, int w, int h,
                                     lv_obj_t** value_label, lv_color_t accent);
    void createDash();
    void createDiag();
    void createTrack();
    void createSettings();
    void createNavigation(lv_obj_t* parent, Page active);
    void createShiftBar(lv_obj_t* parent, lv_obj_t** segments);
    void load(Page page);
    void updateShiftBar(lv_obj_t** segments, uint16_t rpm);
    static void updateValue(lv_obj_t* label, const VehicleState& state,
                            VehicleSignal signal, const char* format);
    static void styleAlarm(lv_obj_t* tile, bool active,
                           AlarmSeverity severity);

    static Ui* instance_;

    Page current_page_ = Page::Dash;
    lv_obj_t* dash_ = nullptr;
    lv_obj_t* diag_ = nullptr;
    lv_obj_t* track_ = nullptr;
    lv_obj_t* settings_ = nullptr;

    lv_obj_t* dash_rpm_ = nullptr;
    lv_obj_t* dash_gear_ = nullptr;
    lv_obj_t* dash_speed_ = nullptr;
    lv_obj_t* dash_map_ = nullptr;
    lv_obj_t* dash_lambda_ = nullptr;
    lv_obj_t* dash_clt_ = nullptr;
    lv_obj_t* dash_iat_ = nullptr;
    lv_obj_t* dash_oil_p_ = nullptr;
    lv_obj_t* dash_oil_t_ = nullptr;
    lv_obj_t* dash_fuel_p_ = nullptr;
    lv_obj_t* dash_batt_ = nullptr;
    lv_obj_t* dash_tps_ = nullptr;
    lv_obj_t* dash_can_ = nullptr;
    lv_obj_t* dash_shift_[12]{};
    lv_obj_t* dash_clt_tile_ = nullptr;
    lv_obj_t* dash_iat_tile_ = nullptr;
    lv_obj_t* dash_oil_p_tile_ = nullptr;
    lv_obj_t* dash_lambda_tile_ = nullptr;
    lv_obj_t* dash_batt_tile_ = nullptr;

    lv_obj_t* diag_status_ = nullptr;
    lv_obj_t* diag_memory_ = nullptr;
    lv_obj_t* diag_runtime_ = nullptr;
    lv_obj_t* diag_can_ = nullptr;

    lv_obj_t* track_rpm_ = nullptr;
    lv_obj_t* track_gear_ = nullptr;
    lv_obj_t* track_speed_ = nullptr;
    lv_obj_t* track_map_ = nullptr;
    lv_obj_t* track_lambda_ = nullptr;
    lv_obj_t* track_oil_p_ = nullptr;
    lv_obj_t* track_clt_ = nullptr;
    lv_obj_t* track_shift_[12]{};

    lv_obj_t* settings_can_ = nullptr;
};
