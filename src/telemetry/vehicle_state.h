#pragma once

#include <cstdint>

struct VehicleState {
    uint16_t rpm = 0;
    int8_t gear = 0;
    float speed_kph = 0.0f;
    float map_bar = 0.0f;
    float lambda = 1.0f;
    float clt_c = 0.0f;
    float iat_c = 0.0f;
    float oil_pressure_bar = 0.0f;
    float oil_temp_c = 0.0f;
    float fuel_pressure_bar = 0.0f;
    float battery_v = 0.0f;
    float tps_percent = 0.0f;
};
