#include <Arduino.h>
#include <lvgl.h>

#include "board/board_display.h"

BoardDisplay board;
bool display_ready = false;

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println();
    Serial.println("OpenDash minimal display diagnostic");

    if (!board.begin()) {
        Serial.println("[DIAG] BoardDisplay::begin FAILED");
        return;
    }
    Serial.println("[DIAG] BoardDisplay::begin OK");

    if (!board.lock()) {
        Serial.println("[DIAG] LVGL lock FAILED");
        return;
    }

    lv_obj_t* screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x00A83A), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title = lv_label_create(screen);
    lv_label_set_text(title, "OPENDASH DISPLAY SELF TEST");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -18);

    lv_obj_t* subtitle = lv_label_create(screen);
    lv_label_set_text(subtitle, "BOARD + LVGL ONLY / NO UI / NO CAN");
    lv_obj_set_style_text_color(subtitle, lv_color_white(), 0);
    lv_obj_align(subtitle, LV_ALIGN_CENTER, 0, 18);

    lv_scr_load(screen);
    board.unlock();

    for (int i = 0; i < 50; ++i) {
        board.service();
        delay(10);
    }

    display_ready = true;
    Serial.println("[DIAG] Minimal screen submitted to LCD");
}

void loop() {
    if (display_ready) {
        board.service();
    }
    delay(2);
}
