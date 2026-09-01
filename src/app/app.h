#pragma once

#include "alarms/alarm_manager.h"
#include "alarms/alarm_runtime.h"
#include "board/board_display.h"
#include "can/can_driver.h"
#include "settings/app_config.h"
#include "settings/nvs_config_store.h"
#include "telemetry/data_source_manager.h"
#include "telemetry/parameter_registry.h"
#include "telemetry/vehicle_state.h"
#include "ui/ui.h"

class App {
public:
    bool begin();
    void loop();

private:
    VehicleState legacyVehicleState() const;
    TelemetryRuntimeStatus telemetryRuntimeStatus() const;
    void applyConfig();

    BoardDisplay board_;
    ParameterRegistry registry_;
    DataSourceManager data_source_{registry_};
    CanDriver can_;
    AlarmManager alarm_manager_;
    AlarmRuntime alarm_runtime_;
    Ui ui_;
    AppConfig config_ = AppConfig::defaults();
    NvsConfigStore config_store_;
    bool ready_ = false;
    uint32_t last_ui_update_ms_ = 0;
};
