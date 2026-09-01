#include <Arduino.h>
#include "app/app.h"

App app;

void setup() {
    Serial.begin(115200);
    delay(250);
    Serial.println();
    Serial.println("BartzDash v0.1");
    app.begin();
}

void loop() {
    app.loop();
}
