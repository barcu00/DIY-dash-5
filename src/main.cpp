#include <Arduino.h>
#include "app/app.h"

App app;

// NVS migration still stages legacy structs on this stack at boot. Runtime
// saves and settings edits keep their AppConfig working copies on the heap.
SET_LOOP_TASK_STACK_SIZE(32768);

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println();
#if defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7)
    Serial.println("DIY Dash - ESP32-S3 Touch-LCD-7");
#else
    Serial.println("DIY Dash - ESP32-S3 Touch-LCD-5");
#endif
    app.begin();
}

void loop() {
    app.loop();
}
