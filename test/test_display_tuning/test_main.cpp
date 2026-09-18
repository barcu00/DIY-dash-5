#include <unity.h>

#include "board/display_tuning.h"

void test_rgb_buffer_holds_one_complete_frame_for_atomic_swap() {
    TEST_ASSERT_EQUAL_UINT32(384000U, DisplayTuning::bufferPixels(800U));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_rgb_buffer_holds_one_complete_frame_for_atomic_swap);
    return UNITY_END();
}
