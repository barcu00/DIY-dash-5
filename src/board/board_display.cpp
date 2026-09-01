#include "board_display.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>

using esp_panel::board::Board;
using esp_panel::drivers::TouchPoint;

namespace {
constexpr uint32_t kLvTickMs = 2;
constexpr uint32_t kBufferLines = 40;
}

bool BoardDisplay::begin() {
    Serial.println("[BartzDash] Initializing board");
    Serial.printf("[BartzDash] PSRAM: %u bytes\n", static_cast<unsigned>(ESP.getPsramSize()));
    Serial.printf("[BartzDash] Flash: %u bytes\n", static_cast<unsigned>(ESP.getFlashChipSize()));

    board_ = new Board();
    if (board_ == nullptr || !board_->init()) {
        Serial.println("[BartzDash] ERROR: board init failed");
        return false;
    }
    if (!board_->begin()) {
        Serial.println("[BartzDash] ERROR: board begin failed");
        return false;
    }

    lcd_ = board_->getLCD();
    touch_ = board_->getTouch();
    if (lcd_ == nullptr) {
        Serial.println("[BartzDash] ERROR: LCD not available");
        return false;
    }

    Serial.printf("[BartzDash] LCD: %ux%u\n",
                  static_cast<unsigned>(lcd_->getFrameWidth()),
                  static_cast<unsigned>(lcd_->getFrameHeight()));

    lv_init();

    const size_t buffer_pixels = static_cast<size_t>(lcd_->getFrameWidth()) * kBufferLines;
    const size_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);

    draw_buf_1_ = static_cast<lv_color_t*>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    draw_buf_2_ = static_cast<lv_color_t*>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (draw_buf_1_ == nullptr) {
        Serial.println("[BartzDash] PSRAM buffer allocation failed, trying internal RAM");
        draw_buf_1_ = static_cast<lv_color_t*>(heap_caps_malloc(buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    if (draw_buf_1_ == nullptr) {
        Serial.println("[BartzDash] ERROR: LVGL buffer allocation failed");
        return false;
    }

    lv_disp_draw_buf_init(&draw_buf_desc_, draw_buf_1_, draw_buf_2_, buffer_pixels);
    lv_disp_drv_init(&disp_drv_);
    disp_drv_.hor_res = lcd_->getFrameWidth();
    disp_drv_.ver_res = lcd_->getFrameHeight();
    disp_drv_.flush_cb = flushCallback;
    disp_drv_.draw_buf = &draw_buf_desc_;
    disp_drv_.user_data = this;
    if (lv_disp_drv_register(&disp_drv_) == nullptr) {
        Serial.println("[BartzDash] ERROR: LVGL display registration failed");
        return false;
    }

    touch_ok_ = touch_ != nullptr && touch_->getPanelHandle() != nullptr;
    if (touch_ok_) {
        lv_indev_drv_init(&indev_drv_);
        indev_drv_.type = LV_INDEV_TYPE_POINTER;
        indev_drv_.read_cb = touchCallback;
        indev_drv_.user_data = this;
        lv_indev_drv_register(&indev_drv_);
        Serial.println("[BartzDash] Touch: GT911 ready");
    } else {
        Serial.println("[BartzDash] WARNING: touch not available");
    }

    lvgl_mutex_ = xSemaphoreCreateRecursiveMutex();
    if (lvgl_mutex_ == nullptr) {
        Serial.println("[BartzDash] ERROR: LVGL mutex allocation failed");
        return false;
    }

    const esp_timer_create_args_t tick_args = {
        .callback = tickCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "bartzdash_lv_tick",
        .skip_unhandled_events = true,
    };
    if (esp_timer_create(&tick_args, &tick_timer_) != ESP_OK ||
        esp_timer_start_periodic(tick_timer_, kLvTickMs * 1000ULL) != ESP_OK) {
        Serial.println("[BartzDash] ERROR: LVGL tick timer failed");
        return false;
    }

    display_ok_ = true;
    Serial.println("[BartzDash] Display/LVGL ready");
    return true;
}

void BoardDisplay::service() {
    if (!display_ok_) {
        return;
    }
    if (lock(20)) {
        lv_timer_handler();
        unlock();
    }
}

bool BoardDisplay::lock(uint32_t timeout_ms) {
    if (lvgl_mutex_ == nullptr) {
        return false;
    }
    return xSemaphoreTakeRecursive(lvgl_mutex_, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void BoardDisplay::unlock() {
    if (lvgl_mutex_ != nullptr) {
        xSemaphoreGiveRecursive(lvgl_mutex_);
    }
}

RuntimeDiagnostics BoardDisplay::diagnostics() const {
    RuntimeDiagnostics d;
    d.display_ok = display_ok_;
    d.touch_ok = touch_ok_;
    d.psram_total = ESP.getPsramSize();
    d.free_heap = ESP.getFreeHeap();
    d.uptime_ms = millis();
    d.ui_updates = ui_updates_;
    return d;
}

void BoardDisplay::incrementUiUpdates() {
    ++ui_updates_;
}

void BoardDisplay::flushCallback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map) {
    auto* self = static_cast<BoardDisplay*>(drv->user_data);
    if (self == nullptr || self->lcd_ == nullptr) {
        lv_disp_flush_ready(drv);
        return;
    }

    const int width = area->x2 - area->x1 + 1;
    const int height = area->y2 - area->y1 + 1;
    self->lcd_->drawBitmap(area->x1, area->y1, width, height, reinterpret_cast<const uint8_t*>(color_map));
    lv_disp_flush_ready(drv);
}

void BoardDisplay::touchCallback(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    auto* self = static_cast<BoardDisplay*>(drv->user_data);
    data->state = LV_INDEV_STATE_RELEASED;
    if (self == nullptr || self->touch_ == nullptr) {
        return;
    }

    TouchPoint point;
    if (self->touch_->readPoints(&point, 1, 0) > 0) {
        data->point.x = point.x;
        data->point.y = point.y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
}

void BoardDisplay::tickCallback(void*) {
    lv_tick_inc(kLvTickMs);
}
