#pragma once

#include <cstdint>


enum class BoardVariant : uint8_t {
    Waveshare5,
    Waveshare7,
};

struct HardwareProfile {
    uint8_t can_tx_gpio;
    uint8_t can_rx_gpio;
    bool has_do0_buzzer;
    bool select_can_with_exio5;
    // Horizontal scale that makes drawn circles round on the physical
    // panel: pixel height / pixel width. The 7 inch panel (154.21 x 85.92 mm
    // active area, 800 x 480) has pixels ~7.7 % wider than tall.
    float round_scale_x;
};

constexpr HardwareProfile hardwareProfile(BoardVariant variant) {
    return variant == BoardVariant::Waveshare7
               ? HardwareProfile{20U, 19U, false, true, (85.92f / 480.0f) / (154.21f / 800.0f)}
               : HardwareProfile{15U, 16U, true, false, 1.0f};
}

constexpr HardwareProfile currentHardwareProfile() {
#if defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7)
    return hardwareProfile(BoardVariant::Waveshare7);
#else
    return hardwareProfile(BoardVariant::Waveshare5);
#endif
}
