#pragma once

#include <cstdint>
#include <lvgl.h>

#include "board/board_display.h"
#include "telemetry/data_source.h"
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
    void begin();
    void update(const VehicleState& state, const RuntimeDiagnostics& diag,
                const TelemetryRuntimeStatus& telemetry);

private:
    static void gestureEvent(lv_event_t* event);
    static lv_obj_t* createValueTile(lv_obj_t* parent, const char* title, int x, int y, int w, int h,
                                     lv_obj_t** value_label, lv_color_t accent);
    void createDash();
    void createDiag();
    void createTrack();
    void createSettings();
    void createShiftBar(lv_obj_t* parent, lv_obj_t** segments);
    void load(UiPage page);
    void updateShiftBar(lv_obj_t** segments, uint16_t rpm);

    static Ui* instance_;

    UiPage current_page_ = UiPage::Dash;
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
    lv_obj_t* dash_source_ = nullptr;
    lv_obj_t* dash_shift_[12]{};

    lv_obj_t* diag_status_ = nullptr;
    lv_obj_t* diag_memory_ = nullptr;
    lv_obj_t* diag_runtime_ = nullptr;
    lv_obj_t* diag_source_ = nullptr;
    lv_obj_t* diag_can_box_ = nullptr;
    lv_obj_t* diag_can_state_ = nullptr;
    lv_obj_t* diag_can_stats_ = nullptr;

    lv_obj_t* track_rpm_ = nullptr;
    lv_obj_t* track_gear_ = nullptr;
    lv_obj_t* track_speed_ = nullptr;
    lv_obj_t* track_map_ = nullptr;
    lv_obj_t* track_lambda_ = nullptr;
    lv_obj_t* track_oil_p_ = nullptr;
    lv_obj_t* track_clt_ = nullptr;
    lv_obj_t* track_shift_[12]{};
};
