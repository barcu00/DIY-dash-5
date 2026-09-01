#include <Arduino.h>
#include <lvgl.h>

#include "app/app.h"
#include "board/board_display.h"

App reserved_app;
BoardDisplay board;

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println("[OpenDash] resident-App memory diagnostic");

    if (!board.begin()) {
        Serial.println("[OpenDash] DISPLAY INIT FAILED WITH FULL APP RESIDENT");
        return;
    }

    if (board.lock()) {
        lv_obj_t* screen = lv_obj_create(nullptr);
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x157A2A), 0);
        lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
        lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* title = lv_label_create(screen);
        lv_label_set_text(title, "APP RESIDENT TEST");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 0, -22);

        lv_obj_t* detail = lv_label_create(screen);
        lv_label_set_text(detail, "FULL APP OBJECT IN RAM / DISPLAY OK");
        lv_obj_set_style_text_font(detail, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(detail, lv_color_white(), 0);
        lv_obj_align(detail, LV_ALIGN_CENTER, 0, 24);

        lv_scr_load(screen);
        board.unlock();
    }

    for (int i = 0; i < 8; ++i) {
        board.service();
        delay(20);
    }
}

void loop() {
    board.service();
    delay(5);
}
