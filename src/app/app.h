#pragma once

#include "alarms/alarm_manager.h"
#include "board/board_display.h"
#include "can/can_driver.h"
#include "config/dashboard_config.h"
#include "ecu/ecu_can_decoder.h"
#include "telemetry/telemetry_manager.h"
#include "ui/ui.h"

class App {
public:
    bool begin();
    void loop();

private:
    BoardDisplay board_;
    CanDriver can_;
    EcuCanDecoder decoder_{nullptr, 0U};
    TelemetryManager telemetry_{decoder_, DashboardConfig::kDemoEnabled,
                                DashboardConfig::kCanTimeoutMs};
    AlarmManager alarms_;
    AlarmSummary alarm_summary_{};
    Ui ui_;
    bool ready_ = false;
    uint32_t last_ui_update_ms_ = 0;
};
