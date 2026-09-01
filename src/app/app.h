#pragma once

#include "board/board_display.h"
#include "telemetry/mock_telemetry.h"
#include "ui/ui.h"

class App {
public:
    bool begin();
    void loop();

private:
    BoardDisplay board_;
    MockTelemetry telemetry_;
    Ui ui_;
    bool ready_ = false;
    uint32_t last_ui_update_ms_ = 0;
};
