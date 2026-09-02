#include <unity.h>

#include "ui/touch_swipe_recognizer.h"

namespace {
void test_left_swipe_emits_once_on_release() {
    TouchSwipeRecognizer recognizer;
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(true, 700, 220, 1000U)));
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(true, 610, 225, 1120U)));
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::Left),
                      static_cast<int>(recognizer.update(false, 0, 0, 1200U)));
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(false, 0, 0, 1210U)));
}

void test_right_swipe_emits_once_on_release() {
    TouchSwipeRecognizer recognizer;
    recognizer.update(true, 100, 200, 0U);
    recognizer.update(true, 190, 195, 180U);
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::Right),
                      static_cast<int>(recognizer.update(false, 0, 0, 220U)));
}

void test_short_horizontal_drag_is_rejected() {
    TouchSwipeRecognizer recognizer;
    recognizer.update(true, 100, 200, 0U);
    recognizer.update(true, 169, 198, 200U);
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(false, 0, 0, 250U)));
}

void test_vertical_drag_is_rejected() {
    TouchSwipeRecognizer recognizer;
    recognizer.update(true, 400, 100, 0U);
    recognizer.update(true, 420, 230, 200U);
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(false, 0, 0, 250U)));
}

void test_nondominant_diagonal_drag_is_rejected() {
    TouchSwipeRecognizer recognizer;
    recognizer.update(true, 100, 100, 0U);
    recognizer.update(true, 180, 170, 200U);
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(false, 0, 0, 250U)));
}

void test_long_drag_is_rejected() {
    TouchSwipeRecognizer recognizer;
    recognizer.update(true, 700, 200, 100U);
    recognizer.update(true, 580, 200, 750U);
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(false, 0, 0, 801U)));
}

void test_new_press_starts_fresh_gesture_after_release() {
    TouchSwipeRecognizer recognizer;
    recognizer.update(true, 700, 200, 0U);
    recognizer.update(true, 600, 200, 100U);
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::Left),
                      static_cast<int>(recognizer.update(false, 0, 0, 150U)));

    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(recognizer.update(true, 100, 220, 300U)));
    recognizer.update(true, 200, 220, 400U);
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::Right),
                      static_cast<int>(recognizer.update(false, 0, 0, 450U)));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_left_swipe_emits_once_on_release);
    RUN_TEST(test_right_swipe_emits_once_on_release);
    RUN_TEST(test_short_horizontal_drag_is_rejected);
    RUN_TEST(test_vertical_drag_is_rejected);
    RUN_TEST(test_nondominant_diagonal_drag_is_rejected);
    RUN_TEST(test_long_drag_is_rejected);
    RUN_TEST(test_new_press_starts_fresh_gesture_after_release);
    return UNITY_END();
}
