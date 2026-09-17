#include <Arduino.h>
#include "app/app.h"

App app;

// Preset banks are data only, but NVS migration and deferred-save snapshots
// temporarily coexist on the Arduino/UI stack. Budget for those copies.
SET_LOOP_TASK_STACK_SIZE(32768);

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println();
    Serial.println("DIY Dash - ESP32-S3 Touch-LCD-5");
    app.begin();
}

void loop() {
    app.loop();
}
