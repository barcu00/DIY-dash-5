#include <unity.h>

#include "board/display_tuning.h"

void test_lvgl_draw_buffer_uses_eighty_lines() {
    TEST_ASSERT_EQUAL_UINT16(80U, DisplayTuning::kBufferLines);
    TEST_ASSERT_EQUAL_UINT32(64000U, DisplayTuning::bufferPixels(800U));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_lvgl_draw_buffer_uses_eighty_lines);
    return UNITY_END();
}
