#include <unity.h>

#include "ui/tile_layout.h"

namespace {
void test_first_six_slots_stack_on_left() {
    for (uint8_t slot = 0; slot < 6U; ++slot) {
        const TilePlacement p = TileLayout::placement(slot);
        TEST_ASSERT_EQUAL_INT16(10, p.x);
        TEST_ASSERT_EQUAL_INT16(42 + static_cast<int16_t>(slot) * 65, p.y);
        TEST_ASSERT_EQUAL_INT16(202, p.width);
        TEST_ASSERT_EQUAL_INT16(58, p.height);
    }
}

void test_second_six_slots_stack_on_right() {
    for (uint8_t slot = 6U; slot < 12U; ++slot) {
        const TilePlacement p = TileLayout::placement(slot);
        TEST_ASSERT_EQUAL_INT16(588, p.x);
        TEST_ASSERT_EQUAL_INT16(42 + static_cast<int16_t>(slot - 6U) * 65, p.y);
    }
}

void test_invalid_slot_normalizes_to_first_slot() {
    const TilePlacement p = TileLayout::placement(99U);
    TEST_ASSERT_EQUAL_INT16(10, p.x);
    TEST_ASSERT_EQUAL_INT16(42, p.y);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_first_six_slots_stack_on_left);
    RUN_TEST(test_second_six_slots_stack_on_right);
    RUN_TEST(test_invalid_slot_normalizes_to_first_slot);
    return UNITY_END();
}
