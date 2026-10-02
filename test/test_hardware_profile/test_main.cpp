#include <unity.h>

#include "board/hardware_profile.h"


void test_five_inch_profile_keeps_existing_external_can_and_buzzer() {
    const auto profile = hardwareProfile(BoardVariant::Waveshare5);

    TEST_ASSERT_EQUAL_UINT8(15U, profile.can_tx_gpio);
    TEST_ASSERT_EQUAL_UINT8(16U, profile.can_rx_gpio);
    TEST_ASSERT_TRUE(profile.has_do0_buzzer);
    TEST_ASSERT_FALSE(profile.select_can_with_exio5);
}

void test_seven_inch_profile_uses_onboard_can_without_do0_buzzer() {
    const auto profile = hardwareProfile(BoardVariant::Waveshare7);

    TEST_ASSERT_EQUAL_UINT8(20U, profile.can_tx_gpio);
    TEST_ASSERT_EQUAL_UINT8(19U, profile.can_rx_gpio);
    TEST_ASSERT_FALSE(profile.has_do0_buzzer);
    TEST_ASSERT_TRUE(profile.select_can_with_exio5);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_five_inch_profile_keeps_existing_external_can_and_buzzer);
    RUN_TEST(test_seven_inch_profile_uses_onboard_can_without_do0_buzzer);
    return UNITY_END();
}
