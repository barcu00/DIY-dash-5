#pragma once

#include "alarms/alarm_manager.h"
#include "board/board_display.h"
#include "can/can_driver.h"
#include "config/dashboard_config.h"
#include "ecu/ecu_can_decoder.h"
#include "telemetry/telemetry_manager.h"
#include "ui/ui.h"
#include "ui/frame_scheduler.h"
#include "settings/app_config.h"
#include "settings/config_repository.h"
#include "settings/nvs_config_backend.h"
#include "alarms/tile_warning_engine.h"
#include "alarms/buzzer_model.h"
#include "alarms/buzzer_test_pulse.h"
#include "racechrono/racechrono_telemetry.h"

class App {
public:
    bool begin();
    void loop();

private:
    void applyRuntimeConfig(uint32_t now_ms);

    BoardDisplay board_;
    CanDriver can_;
    EcuCanDecoder decoder_{nullptr, 0U};
    TelemetryManager telemetry_{decoder_, DashboardConfig::kCanTimeoutMs};
    RaceChronoTelemetry racechrono_{};
    NvsConfigBackend config_backend_{};
    ConfigRepository config_repository_{config_backend_};
    AppConfig config_ = AppConfig::defaults();
    TileWarningEngine warnings_{};
    BuzzerModel buzzer_{};
    BuzzerTestPulse buzzer_test_{};
    Ui ui_;
    FrameScheduler frame_scheduler_{};
    bool ready_ = false;
};
