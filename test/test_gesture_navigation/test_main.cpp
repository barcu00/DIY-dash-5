#include <unity.h>

#include "ui/gesture_navigation.h"

namespace {
void test_swipe_left_moves_to_next_page() {
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Diag),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Dash, GestureDirection::Left)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Track),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Diag, GestureDirection::Left)));
}

void test_swipe_right_moves_to_previous_page() {
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Diag),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Track, GestureDirection::Right)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Dash),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Diag, GestureDirection::Right)));
}

void test_navigation_wraps_without_indicator_state() {
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Dash),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Track, GestureDirection::Left)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Track),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Dash, GestureDirection::Right)));
}

void test_vertical_or_none_does_not_change_page() {
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Diag),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Diag, GestureDirection::Up)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Diag),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Diag, GestureDirection::Down)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Diag),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Diag, GestureDirection::None)));
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_swipe_left_moves_to_next_page);
    RUN_TEST(test_swipe_right_moves_to_previous_page);
    RUN_TEST(test_navigation_wraps_without_indicator_state);
    RUN_TEST(test_vertical_or_none_does_not_change_page);
    return UNITY_END();
}
