#pragma once

#include "board/board_display.h"
#include "can/can_driver.h"
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

    BoardDisplay board_;
    ParameterRegistry registry_;
    DataSourceManager data_source_{registry_};
    CanDriver can_;
    Ui ui_;
    bool ready_ = false;
    uint32_t last_ui_update_ms_ = 0;
};
