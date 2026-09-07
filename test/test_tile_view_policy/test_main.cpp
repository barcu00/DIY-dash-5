#include <unity.h>

#include "ui/tile_view_policy.h"

void test_small_and_wide_tile_values_are_centered_and_white() {
    const TileViewPolicy small = tileViewPolicy(TileSize::Small);
    const TileViewPolicy wide = tileViewPolicy(TileSize::Wide);

    TEST_ASSERT_TRUE(small.value_centered);
    TEST_ASSERT_TRUE(wide.value_centered);
    TEST_ASSERT_EQUAL_HEX32(0xF2F5F7U, small.value_rgb);
    TEST_ASSERT_EQUAL_HEX32(0xF2F5F7U, wide.value_rgb);
}

void test_tile_editor_requires_long_press() {
    const TileViewPolicy policy = tileViewPolicy(TileSize::Small);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TileOpenGesture::LongPress),
                            static_cast<uint8_t>(policy.open_gesture));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(500U, policy.long_press_ms);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_small_and_wide_tile_values_are_centered_and_white);
    RUN_TEST(test_tile_editor_requires_long_press);
    return UNITY_END();
}
