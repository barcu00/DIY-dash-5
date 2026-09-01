#include <unity.h>

#include "ui/tile_slot_navigation.h"

void setUp() {}
void tearDown() {}

void test_next_wraps_all_twelve_slots() {
    TEST_ASSERT_EQUAL_UINT8(1U, TileSlotNavigation::next(0U));
    TEST_ASSERT_EQUAL_UINT8(0U, TileSlotNavigation::next(11U));
}

void test_previous_wraps_all_twelve_slots() {
    TEST_ASSERT_EQUAL_UINT8(11U, TileSlotNavigation::previous(0U));
    TEST_ASSERT_EQUAL_UINT8(10U, TileSlotNavigation::previous(11U));
}

void test_invalid_slot_is_normalized() {
    TEST_ASSERT_EQUAL_UINT8(0U, TileSlotNavigation::normalize(12U));
    TEST_ASSERT_EQUAL_UINT8(0U, TileSlotNavigation::normalize(255U));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_next_wraps_all_twelve_slots);
    RUN_TEST(test_previous_wraps_all_twelve_slots);
    RUN_TEST(test_invalid_slot_is_normalized);
    return UNITY_END();
}
