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
    void setSoftwareBrightness(uint8_t percent);
    bool beginBuzzer();
    void setBuzzer(bool on);

private:
    static void flushCallback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map);
    static void touchCallback(lv_indev_drv_t* drv, lv_indev_data_t* data);
    static void tickCallback(void* arg);
    static bool IRAM_ATTR refreshCallback(void* arg);
    static void monitorCallback(lv_disp_drv_t* drv, uint32_t time_ms, uint32_t pixels);
    static void logCallback(const char* text);

    esp_panel::board::Board* board_ = nullptr;
    esp_panel::drivers::LCD* lcd_ = nullptr;
    esp_panel::drivers::Touch* touch_ = nullptr;

    lv_color_t* draw_buf_1_ = nullptr;
    lv_color_t* draw_buf_2_ = nullptr;
    lv_disp_draw_buf_t draw_buf_desc_{};
    lv_disp_drv_t disp_drv_{};
    lv_indev_drv_t indev_drv_{};

    SemaphoreHandle_t lvgl_mutex_ = nullptr;
    SemaphoreHandle_t vsync_sem_ = nullptr;
    uint32_t frame_count_ = 0;
    uint32_t frame_total_ms_ = 0;
    uint32_t frame_max_ms_ = 0;
    uint32_t last_diagnostics_ms_ = 0;
    uint32_t max_service_us_ = 0;
    esp_timer_handle_t tick_timer_ = nullptr;

    bool display_ok_ = false;
    bool touch_ok_ = false;
    uint32_t ui_updates_ = 0;
    lv_obj_t* brightness_layer_ = nullptr;
    uint8_t brightness_percent_ = 0;
    bool buzzer_ready_ = false;
    bool buzzer_on_ = false;
};
