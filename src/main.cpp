#include <Arduino.h>
#include <array>
#include <lvgl.h>

#include "board/board_display.h"

namespace {
BoardDisplay board;
lv_obj_t* screen = nullptr;
lv_obj_t* stage_label = nullptr;
std::array<lv_obj_t*, 12> tiles{};
std::array<lv_obj_t*, 12> canvases{};
std::array<std::array<lv_color_t, 16U * 16U>, 24> canvas_buffers{};
uint8_t stage = 0;
uint32_t next_stage_ms = 0;

void setStageText(const char* text, lv_color_t color) {
    if (stage_label == nullptr) return;
    lv_label_set_text(stage_label, text);
    lv_obj_set_style_text_color(stage_label, color, 0);
}

void createBaseScreen() {
    screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x072812), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "OPENDASH UI STAGED TEST");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    stage_label = lv_label_create(screen);
    lv_label_set_text(stage_label, "STAGE 1 / DISPLAY OK");
    lv_obj_set_style_text_font(stage_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(stage_label, lv_color_hex(0x39D353), 0);
    lv_obj_align(stage_label, LV_ALIGN_TOP_MID, 0, 60);

    lv_scr_load(screen);
}

void addDashSkeleton() {
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07090D), 0);
    setStageText("STAGE 2 / DASH SKELETON", lv_color_hex(0x20A4F3));

    lv_obj_t* rpm = lv_label_create(screen);
    lv_label_set_text(rpm, "0000");
    lv_obj_set_style_text_font(rpm, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(rpm, lv_color_white(), 0);
    lv_obj_set_pos(rpm, 286, 100);

    lv_obj_t* gear = lv_label_create(screen);
    lv_label_set_text(gear, "N");
    lv_obj_set_style_text_font(gear, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(gear, lv_color_hex(0xFFCF33), 0);
    lv_obj_set_pos(gear, 432, 100);

    for (int i = 0; i < 12; ++i) {
        lv_obj_t* seg = lv_obj_create(screen);
        lv_obj_set_pos(seg, 14 + i * 64, 84);
        lv_obj_set_size(seg, 52, 16);
        lv_obj_set_style_border_width(seg, 0, 0);
        lv_obj_set_style_radius(seg, 4, 0);
        lv_obj_set_style_bg_color(seg, lv_color_hex(0x202830), 0);
        lv_obj_clear_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
    }
}

void addTiles() {
    setStageText("STAGE 3 / 12 TILE OBJECTS", lv_color_hex(0xB26BFF));
    for (uint8_t i = 0; i < 12; ++i) {
        lv_obj_t* tile = lv_obj_create(screen);
        tiles[i] = tile;
        const bool right = i >= 6U;
        const uint8_t slot = i % 6U;
        lv_obj_set_pos(tile, right ? 590 : 10, 170 + slot * 48);
        lv_obj_set_size(tile, 200, 42);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x11161D), 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(0x27313C), 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        lv_obj_set_style_radius(tile, 6, 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* label = lv_label_create(tile);
        lv_label_set_text_fmt(label, "TILE %u", static_cast<unsigned>(i + 1U));
        lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_pos(label, 8, 8);
    }
}

void addCanvases() {
    setStageText("STAGE 4 / 12 CANVASES", lv_color_hex(0xFFCF33));
    for (uint8_t i = 0; i < 12; ++i) {
        lv_obj_t* canvas = lv_canvas_create(tiles[i]);
        canvases[i] = canvas;
        lv_canvas_set_buffer(canvas, canvas_buffers[i].data(), 16, 16, LV_IMG_CF_TRUE_COLOR);
        lv_canvas_fill_bg(canvas, lv_color_hex(0x11161D), LV_OPA_COVER);
        for (uint8_t p = 2; p < 14; ++p) {
            lv_canvas_set_px_color(canvas, p, p, lv_color_hex(0x20A4F3));
            lv_canvas_set_px_color(canvas, static_cast<uint8_t>(15U - p), p, lv_color_hex(0x39D353));
        }
        lv_obj_set_pos(canvas, 170, 8);
    }
}

void addSecondPage() {
    setStageText("STAGE 5 / 24 CANVASES TOTAL", lv_color_hex(0xFF7A33));
    lv_obj_t* page = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x07090D), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t i = 0; i < 12; ++i) {
        lv_obj_t* tile = lv_obj_create(page);
        const bool right = i >= 6U;
        const uint8_t slot = i % 6U;
        lv_obj_set_pos(tile, right ? 590 : 10, 42 + slot * 66);
        lv_obj_set_size(tile, 200, 60);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x11161D), 0);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* canvas = lv_canvas_create(tile);
        lv_canvas_set_buffer(canvas, canvas_buffers[12U + i].data(), 16, 16, LV_IMG_CF_TRUE_COLOR);
        lv_canvas_fill_bg(canvas, lv_color_hex(0x11161D), LV_OPA_COVER);
        lv_canvas_set_px_color(canvas, 8, 8, lv_color_hex(0xB26BFF));
        lv_obj_set_pos(canvas, 170, 7);
    }
}

void addSettingsObjects() {
    setStageText("STAGE 6 / SETTINGS OBJECTS", lv_color_hex(0xFF3B30));
    lv_obj_t* page = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x07090D), 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    for (uint8_t i = 0; i < 7; ++i) {
        lv_obj_t* card = lv_obj_create(page);
        const int col = i % 2;
        const int row = i / 2;
        lv_obj_set_pos(card, 24 + col * 382, 92 + row * 86);
        lv_obj_set_size(card, 360, 70);
        lv_obj_set_style_bg_color(card, lv_color_hex(0x11161D), 0);
        lv_obj_set_style_border_color(card, lv_color_hex(0x27313C), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 8, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* label = lv_label_create(card);
        lv_label_set_text_fmt(label, "SETTINGS %u", static_cast<unsigned>(i + 1U));
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_pos(label, 16, 18);
    }
}

void runNextStage() {
    if (!board.lock()) return;
    ++stage;
    switch (stage) {
        case 2: addDashSkeleton(); break;
        case 3: addTiles(); break;
        case 4: addCanvases(); break;
        case 5: addSecondPage(); break;
        case 6: addSettingsObjects(); break;
        case 7:
            setStageText("STAGE 7 / COMPLETE - ALL UI OBJECTS SURVIVED", lv_color_hex(0x39D353));
            break;
        default: break;
    }
    board.unlock();
}
}

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println("[OpenDash] staged UI self-test");
    if (!board.begin()) {
        Serial.println("[OpenDash] display init failed");
        return;
    }
    if (board.lock()) {
        createBaseScreen();
        board.unlock();
    }
    stage = 1;
    next_stage_ms = millis() + 2000U;
}

void loop() {
    board.service();
    if (stage >= 1U && stage < 7U && static_cast<int32_t>(millis() - next_stage_ms) >= 0) {
        runNextStage();
        next_stage_ms = millis() + 2000U;
    }
    delay(2);
}
