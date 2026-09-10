#include <Arduino.h>
#include "app/app.h"

App app;

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
