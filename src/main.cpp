#include <Arduino.h>
#include <lvgl.h>

#include "board/board_display.h"
#include "settings/app_config.h"
#include "settings/nvs_config_store.h"
#include "telemetry/parameter_registry.h"
#include "telemetry/vehicle_state.h"
#include "ui/ui.h"

namespace {
BoardDisplay board;
ParameterRegistry registry;
AppConfig config = AppConfig::defaults();
NvsConfigStore store;
Ui ui;

uint8_t stage = 0;
uint32_t next_stage_ms = 0;

void createStageOne() {
    lv_obj_t* screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x157A2A), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "OPENDASH REAL UI TEST");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -28);

    lv_obj_t* detail = lv_label_create(screen);
    lv_label_set_text(detail, "STAGE 1 / DISPLAY + APP OBJECTS OK");
    lv_obj_set_style_text_font(detail, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(detail, lv_color_white(), 0);
    lv_obj_align(detail, LV_ALIGN_CENTER, 0, 20);

    lv_scr_load(screen);
}

void stageBanner(const char* text, lv_color_t color) {
    lv_obj_t* banner = lv_obj_create(lv_layer_top());
    lv_obj_set_size(banner, 520, 54);
    lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_bg_color(banner, lv_color_hex(0x090D12), 0);
    lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(banner, color, 0);
    lv_obj_set_style_border_width(banner, 2, 0);
    lv_obj_set_style_radius(banner, 8, 0);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* label = lv_label_create(banner);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_center(label);
}

void runUiBegin() {
    Serial.println("[OpenDash diagnostic] entering Ui::begin()");
    if (!board.lock()) return;
    ui.begin(config, registry, store);
    stageBanner("STAGE 2 / Ui::begin() RETURNED", lv_color_hex(0x20A4F3));
    board.unlock();
    Serial.println("[OpenDash diagnostic] Ui::begin() returned");
}

void runFirstUpdate() {
    Serial.println("[OpenDash diagnostic] entering first Ui::update()");
    VehicleState state{};
    state.rpm = 3250;
    state.gear = 3;

    TelemetryRuntimeStatus telemetry{};
    telemetry.source = DataSource::Mock;
    telemetry.can_bitrate = 1000000U;

    if (!board.lock()) return;
    ui.update(state, board.diagnostics(), telemetry);
    stageBanner("STAGE 3 / FIRST Ui::update() RETURNED", lv_color_hex(0x39D353));
    board.unlock();
    Serial.println("[OpenDash diagnostic] first Ui::update() returned");
}
}

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println("[OpenDash] real UI staged diagnostic");

    config.validate();

    if (!board.begin()) {
        Serial.println("[OpenDash] display init failed");
        return;
    }

    if (board.lock()) {
        createStageOne();
        board.unlock();
    }

    for (uint8_t i = 0; i < 8U; ++i) {
        board.service();
        delay(20);
    }

    stage = 1U;
    next_stage_ms = millis() + 2500U;
}

void loop() {
    board.service();

    if (stage == 1U && static_cast<int32_t>(millis() - next_stage_ms) >= 0) {
        runUiBegin();
        stage = 2U;
        next_stage_ms = millis() + 3000U;
    } else if (stage == 2U && static_cast<int32_t>(millis() - next_stage_ms) >= 0) {
        runFirstUpdate();
        stage = 3U;
    }

    delay(2);
}
