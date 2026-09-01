#pragma once

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>

struct RuntimeDiagnostics {
    bool display_ok = false;
    bool touch_ok = false;
    size_t psram_total = 0;
    size_t free_heap = 0;
    uint32_t uptime_ms = 0;
    uint32_t ui_updates = 0;
};

class BoardDisplay {
public:
    bool begin();
    void service();
    bool lock(uint32_t timeout_ms = 1000);
    void unlock();

    RuntimeDiagnostics diagnostics() const;
    void incrementUiUpdates();

private:
    static void flushCallback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map);
    static void touchCallback(lv_indev_drv_t* drv, lv_indev_data_t* data);
    static void tickCallback(void* arg);

    esp_panel::board::Board* board_ = nullptr;
    esp_panel::drivers::LCD* lcd_ = nullptr;
    esp_panel::drivers::Touch* touch_ = nullptr;

    lv_color_t* draw_buf_1_ = nullptr;
    lv_color_t* draw_buf_2_ = nullptr;
    lv_disp_draw_buf_t draw_buf_desc_{};
    lv_disp_drv_t disp_drv_{};
    lv_indev_drv_t indev_drv_{};

    SemaphoreHandle_t lvgl_mutex_ = nullptr;
    esp_timer_handle_t tick_timer_ = nullptr;

    bool display_ok_ = false;
    bool touch_ok_ = false;
    uint32_t ui_updates_ = 0;
};
