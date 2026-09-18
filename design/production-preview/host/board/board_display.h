#pragma once
#include <cstddef>
#include <cstdint>
struct RuntimeDiagnostics {
    bool display_ok=false,touch_ok=false;
    size_t psram_total=0,free_heap=0;
    uint32_t uptime_ms=0,ui_updates=0;
};
// Only the physical brightness output is substituted; UI and LVGL are real.
class BoardDisplay {
public:
    void setSoftwareBrightness(uint8_t) {}
};
