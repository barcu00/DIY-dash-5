#pragma once

#include <lvgl.h>
#include "board/board_display.h"
#include "telemetry/vehicle_state.h"

class Ui {
public:
    void begin();
    void update(const VehicleState& state, const RuntimeDiagnostics& diag);

private:
    enum class Page : uint8_t { Dash, Diag, Track };

    static void navEvent(lv_event_t* event);
    static lv_obj_t* createValueTile(lv_obj_t* parent, const char* title, int x, int y, int w, int h,
                                     lv_obj_t** value_label, lv_color_t accent);
    void createDash();
    void createDiag();
    void createTrack();
    void createNavigation(lv_obj_t* parent, Page active);
    void createShiftBar(lv_obj_t* parent, lv_obj_t** segments);
    void load(Page page);
    void updateShiftBar(lv_obj_t** segments, uint16_t rpm);

    static Ui* instance_;

    Page current_page_ = Page::Dash;
    lv_obj_t* dash_ = nullptr;
    lv_obj_t* diag_ = nullptr;
    lv_obj_t* track_ = nullptr;

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
    lv_obj_t* dash_shift_[12]{};

    lv_obj_t* diag_status_ = nullptr;
    lv_obj_t* diag_memory_ = nullptr;
    lv_obj_t* diag_runtime_ = nullptr;

    lv_obj_t* track_rpm_ = nullptr;
    lv_obj_t* track_gear_ = nullptr;
    lv_obj_t* track_speed_ = nullptr;
    lv_obj_t* track_map_ = nullptr;
    lv_obj_t* track_lambda_ = nullptr;
    lv_obj_t* track_oil_p_ = nullptr;
    lv_obj_t* track_clt_ = nullptr;
    lv_obj_t* track_shift_[12]{};
};
