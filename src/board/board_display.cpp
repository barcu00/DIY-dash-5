#include "board_display.h"

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <algorithm>
#include <cstring>

#include "board/display_tuning.h"
#include "board/rgb_sdk_requirements.h"

using esp_panel::board::Board;
using esp_panel::drivers::TouchPoint;
uintptr_t diy_lvgl_memory = 0;

bool BoardDisplay::beginBuzzer() {
    auto* adapter = board_ ? board_->getIO_Expander() : nullptr;
    auto* expander = adapter ? adapter->getBase() : nullptr;
    // CH422G OC0 maps to logical pin 8, not ESP32 GPIO8 (I2C SDA).
    // The optocoupler LED is connected to 3V3: LOW energizes DO0.
    buzzer_ready_ = expander && expander->digitalWrite(8, HIGH) &&
                    expander->pinMode(8, OUTPUT);
    buzzer_on_ = false;
    return buzzer_ready_;
}

void BoardDisplay::setBuzzer(bool on) {
    if (!buzzer_ready_ || on == buzzer_on_) return;
    if (board_->getIO_Expander()->getBase()->digitalWrite(8, on ? LOW : HIGH))
        buzzer_on_ = on; // failed writes are retried on the next loop
}

namespace {
constexpr uint32_t kLvTickMs = 2;
constexpr uint16_t kExpectedWidth = 800;
constexpr uint16_t kExpectedHeight = 480;
}

bool BoardDisplay::begin() {
    Serial.println("[DIY Dash] Initializing board");
    Serial.printf("[DIY Dash] PSRAM: %u bytes\n", static_cast<unsigned>(ESP.getPsramSize()));
    Serial.printf("[DIY Dash] Flash: %u bytes\n", static_cast<unsigned>(ESP.getFlashChipSize()));
    if (ESP.getPsramSize() == 0U) {
        Serial.println("[DIY Dash] WARNING: PSRAM not detected");
    }

    board_ = new Board();
    if (board_ == nullptr || !board_->init()) {
        Serial.println("[DIY Dash] ERROR: board init failed");
        return false;
    }
    lcd_ = board_->getLCD();
    // Configure before Board::begin initializes the panel. Use the actual
    // two LCD framebuffers, not two small scratch buffers copied into GRAM.
    if (!lcd_ || !lcd_->configFrameBufferNumber(2)) {
        Serial.println("[DIY Dash] ERROR: RGB double-buffer configuration failed");
        return false;
    }
    if (!board_->begin()) {
        Serial.println("[DIY Dash] ERROR: board begin failed");
        return false;
    }

    lcd_ = board_->getLCD();
    touch_ = board_->getTouch();
    if (lcd_ == nullptr) {
        Serial.println("[DIY Dash] ERROR: LCD not available");
        return false;
    }

    Serial.printf("[DIY Dash] LCD: %ux%u\n",
                  static_cast<unsigned>(lcd_->getFrameWidth()),
                  static_cast<unsigned>(lcd_->getFrameHeight()));
    if (lcd_->getFrameWidth() != kExpectedWidth ||
        lcd_->getFrameHeight() != kExpectedHeight) {
        Serial.println("[DIY Dash] ERROR: unexpected LCD resolution");
        return false;
    }

    diy_lvgl_memory = reinterpret_cast<uintptr_t>(heap_caps_malloc(
        LV_MEM_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!diy_lvgl_memory) {
        Serial.println("[DIY Dash] ERROR: LVGL PSRAM pool allocation failed");
        return false;
    }
    lv_init();
    lv_log_register_print_cb(logCallback);

    const size_t buffer_pixels =
        DisplayTuning::bufferPixels(lcd_->getFrameWidth());
    const size_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);

    draw_buf_1_ = static_cast<lv_color_t*>(lcd_->getFrameBufferByIndex(0));
    draw_buf_2_ = static_cast<lv_color_t*>(lcd_->getFrameBufferByIndex(1));
    if (!draw_buf_1_ || !draw_buf_2_) {
        Serial.println("[DIY Dash] ERROR: LVGL buffer allocation failed");
        return false;
    }
    std::memset(draw_buf_1_,0,buffer_bytes);
    std::memset(draw_buf_2_,0,buffer_bytes);
    // LVGL must start drawing into the buffer not currently scanned by LCD.
    vsync_sem_=xSemaphoreCreateBinary();
    if (!vsync_sem_ || !lcd_->attachRefreshFinishCallback(refreshCallback,this) ||
        !lcd_->switchFrameBufferTo(draw_buf_2_)) {
        Serial.println("[DIY Dash] ERROR: RGB VSYNC initialization failed");
        return false;
    }
    xSemaphoreTake(vsync_sem_,0);
    if(xSemaphoreTake(vsync_sem_,pdMS_TO_TICKS(100))!=pdTRUE) {
        Serial.println("[DIY Dash] ERROR: initial VSYNC timeout");
        return false;
    }

    lv_disp_draw_buf_init(&draw_buf_desc_, draw_buf_1_, draw_buf_2_, buffer_pixels);
    lv_disp_drv_init(&disp_drv_);
    disp_drv_.hor_res = lcd_->getFrameWidth();
    disp_drv_.ver_res = lcd_->getFrameHeight();
    disp_drv_.flush_cb = flushCallback;
    disp_drv_.draw_buf = &draw_buf_desc_;
    disp_drv_.user_data = this;
    // LVGL 8.4 copies prior dirty regions between these full buffers itself.
    disp_drv_.direct_mode = 1;
    disp_drv_.monitor_cb = monitorCallback;
    if (lv_disp_drv_register(&disp_drv_) == nullptr) {
        Serial.println("[DIY Dash] ERROR: LVGL display registration failed");
        return false;
    }

    touch_ok_ = touch_ != nullptr && touch_->getPanelHandle() != nullptr;
    if (touch_ok_) {
        lv_indev_drv_init(&indev_drv_);
        indev_drv_.type = LV_INDEV_TYPE_POINTER;
        indev_drv_.read_cb = touchCallback;
        indev_drv_.user_data = this;
        indev_drv_.long_press_time = 600U;
        lv_indev_drv_register(&indev_drv_);
        Serial.println("[DIY Dash] Touch: GT911 ready");
    } else {
        Serial.println("[DIY Dash] WARNING: touch not available");
    }

    lvgl_mutex_ = xSemaphoreCreateRecursiveMutex();
    if (lvgl_mutex_ == nullptr) {
        Serial.println("[DIY Dash] ERROR: LVGL mutex allocation failed");
        return false;
    }

    const esp_timer_create_args_t tick_args = {
        .callback = tickCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "diy_dash_lv_tick",
        .skip_unhandled_events = true,
    };
    if (esp_timer_create(&tick_args, &tick_timer_) != ESP_OK ||
        esp_timer_start_periodic(tick_timer_, kLvTickMs * 1000ULL) != ESP_OK) {
        Serial.println("[DIY Dash] ERROR: LVGL tick timer failed");
        return false;
    }

    display_ok_ = true;
    Serial.println("[DIY Dash] Display/LVGL ready");
    return true;
}

void BoardDisplay::service() {
    if (!display_ok_) {
        return;
    }
    if (lock(20)) {
        const int64_t start=esp_timer_get_time();
        lv_timer_handler();
        max_service_us_=std::max(max_service_us_,static_cast<uint32_t>(esp_timer_get_time()-start));
        if (millis()-last_diagnostics_ms_>=5000U) {
            lv_mem_monitor_t memory;lv_mem_monitor(&memory);
            Serial.printf("[UI] frames=%u avg=%u ms max=%u ms handler_max=%u us LVGL_free=%u largest=%u frag=%u%% heap=%u psram_free=%u stack_free=%u\n",
                static_cast<unsigned>(frame_count_),static_cast<unsigned>(frame_count_ ? frame_total_ms_/frame_count_:0),
                static_cast<unsigned>(frame_max_ms_),static_cast<unsigned>(max_service_us_),
                static_cast<unsigned>(memory.free_size),static_cast<unsigned>(memory.free_biggest_size),
                static_cast<unsigned>(memory.frag_pct),static_cast<unsigned>(ESP.getFreeHeap()),
                static_cast<unsigned>(ESP.getFreePsram()),static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
            last_diagnostics_ms_=millis();frame_count_=frame_total_ms_=frame_max_ms_=max_service_us_=0;
        }
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

void BoardDisplay::setSoftwareBrightness(uint8_t percent) {
    percent = constrain(percent, 20U, 100U);
    if (brightness_layer_ && percent==brightness_percent_)return;
    brightness_percent_=percent;
    if (brightness_layer_ == nullptr) {
        brightness_layer_ = lv_obj_create(lv_layer_top());
        lv_obj_set_pos(brightness_layer_, 0, 0);
        lv_obj_set_size(brightness_layer_, kExpectedWidth, kExpectedHeight);
        lv_obj_set_style_bg_color(brightness_layer_, lv_color_black(), 0);
        lv_obj_set_style_border_width(brightness_layer_, 0, 0);
        lv_obj_set_style_radius(brightness_layer_, 0, 0);
        lv_obj_clear_flag(brightness_layer_, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(brightness_layer_, LV_OBJ_FLAG_SCROLLABLE);
    }
    const uint8_t opacity = static_cast<uint8_t>(
        ((100U - percent) * static_cast<uint16_t>(LV_OPA_80)) / 80U);
    lv_obj_set_style_bg_opa(brightness_layer_, opacity, 0);
    lv_obj_move_foreground(brightness_layer_);
}

void BoardDisplay::flushCallback(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_map) {
    auto* self = static_cast<BoardDisplay*>(drv->user_data);
    if (self == nullptr || self->lcd_ == nullptr) {
        lv_disp_flush_ready(drv);
        return;
    }

    if (lv_disp_flush_is_last(drv)) {
        if (!self->lcd_->switchFrameBufferTo(color_map)) {
            Serial.println("[UI] ERROR: RGB framebuffer switch failed");
            abort();
        }
        // Clear AFTER scheduling the swap, then wait for the next refresh.
        // Never reuse a buffer still scanned by RGB, or wait indefinitely.
        xSemaphoreTake(self->vsync_sem_,0);
        if (xSemaphoreTake(self->vsync_sem_,pdMS_TO_TICKS(100))!=pdTRUE) {
            Serial.println("[UI] ERROR: VSYNC timeout; aborting unsafe buffer reuse");
            abort();
        }
    }
    lv_disp_flush_ready(drv);
}

bool IRAM_ATTR BoardDisplay::refreshCallback(void* arg) {
    auto* self=static_cast<BoardDisplay*>(arg);
    BaseType_t wake=pdFALSE;
    if(self && self->vsync_sem_)xSemaphoreGiveFromISR(self->vsync_sem_,&wake);
    return wake==pdTRUE;
}
void BoardDisplay::monitorCallback(lv_disp_drv_t* drv,uint32_t time_ms,uint32_t) {
    auto* self=static_cast<BoardDisplay*>(drv->user_data);
    ++self->frame_count_;self->frame_total_ms_+=time_ms;
    self->frame_max_ms_=std::max(self->frame_max_ms_,time_ms);
}
void BoardDisplay::logCallback(const char* text) {
    Serial.print("[LVGL] ");Serial.print(text);
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
