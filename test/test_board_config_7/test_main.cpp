#include <unity.h>

#define BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_7
#include "config/dashboard_config.h"

void setUp() {}
void tearDown() {}

void test_waveshare_7_uses_shared_usb_can_pins() {
    TEST_ASSERT_EQUAL_UINT8(20U, DashboardConfig::kCanTxGpio);
    TEST_ASSERT_EQUAL_UINT8(19U, DashboardConfig::kCanRxGpio);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_waveshare_7_uses_shared_usb_can_pins);
    return UNITY_END();
}
