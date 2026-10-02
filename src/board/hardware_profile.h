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
};

constexpr HardwareProfile hardwareProfile(BoardVariant variant) {
    return variant == BoardVariant::Waveshare7
               ? HardwareProfile{20U, 19U, false, true}
               : HardwareProfile{15U, 16U, true, false};
}

constexpr HardwareProfile currentHardwareProfile() {
#if defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7)
    return hardwareProfile(BoardVariant::Waveshare7);
#else
    return hardwareProfile(BoardVariant::Waveshare5);
#endif
}
