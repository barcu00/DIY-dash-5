#include <unity.h>

#include "ui/flag_tile_model.h"

void test_unavailable_flag_has_no_active_color() {
    const FlagTilePresentation presentation = flagTilePresentation(
        SignalValue{}, FlagActiveColor::Red);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(FlagTileState::Unavailable),
                            static_cast<uint8_t>(presentation.state));
    TEST_ASSERT_EQUAL_STRING("UNAVAILABLE", presentation.status_text);
    TEST_ASSERT_FALSE(presentation.active_tint);
}

void test_off_flag_uses_neutral_variant_c_presentation() {
    const FlagTilePresentation presentation = flagTilePresentation(
        SignalValue{0.0f, 10U, true}, FlagActiveColor::Green);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(FlagTileState::Off),
                            static_cast<uint8_t>(presentation.state));
    TEST_ASSERT_EQUAL_STRING("OFF", presentation.status_text);
    TEST_ASSERT_FALSE(presentation.active_tint);
    TEST_ASSERT_EQUAL_HEX32(0x737E87U, presentation.pill_rgb);
}

void test_on_flag_uses_each_configured_active_color_and_tint() {
    const FlagTilePresentation yellow = flagTilePresentation(
        SignalValue{1.0f, 10U, true}, FlagActiveColor::Yellow);
    const FlagTilePresentation green = flagTilePresentation(
        SignalValue{1.0f, 10U, true}, FlagActiveColor::Green);
    const FlagTilePresentation red = flagTilePresentation(
        SignalValue{1.0f, 10U, true}, FlagActiveColor::Red);

    TEST_ASSERT_EQUAL_STRING("ON", yellow.status_text);
    TEST_ASSERT_TRUE(yellow.active_tint);
    TEST_ASSERT_EQUAL_HEX32(0xFFD166U, yellow.rail_rgb);
    TEST_ASSERT_EQUAL_HEX32(0x2FE38CU, green.rail_rgb);
    TEST_ASSERT_EQUAL_HEX32(0xFF4D5AU, red.rail_rgb);
    TEST_ASSERT_NOT_EQUAL(yellow.background_rgb, green.background_rgb);
    TEST_ASSERT_NOT_EQUAL(green.background_rgb, red.background_rgb);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_unavailable_flag_has_no_active_color);
    RUN_TEST(test_off_flag_uses_neutral_variant_c_presentation);
    RUN_TEST(test_on_flag_uses_each_configured_active_color_and_tint);
    return UNITY_END();
}
