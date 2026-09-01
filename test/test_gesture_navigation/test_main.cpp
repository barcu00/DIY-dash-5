#include <unity.h>

#include "ui/gesture_navigation.h"

namespace {
void test_swipe_left_follows_screen_ring() {
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Track),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Dash, GestureDirection::Left)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Diag),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Track, GestureDirection::Left)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Settings),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Diag, GestureDirection::Left)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Dash),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Settings, GestureDirection::Left)));
}

void test_swipe_right_follows_reverse_screen_ring() {
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Settings),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Dash, GestureDirection::Right)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Diag),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Settings, GestureDirection::Right)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Track),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Diag, GestureDirection::Right)));
    TEST_ASSERT_EQUAL(static_cast<int>(UiPage::Dash),
                      static_cast<int>(GestureNavigation::navigate(UiPage::Track, GestureDirection::Right)));
}

void test_classifier_accepts_clear_horizontal_swipes() {
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::Left),
                      static_cast<int>(GestureNavigation::classify(-100, 15, 250U)));
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::Right),
                      static_cast<int>(GestureNavigation::classify(90, -10, 300U)));
}

void test_classifier_rejects_short_vertical_and_long_press_movements() {
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(GestureNavigation::classify(30, 5, 200U)));
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(GestureNavigation::classify(30, 90, 250U)));
    TEST_ASSERT_EQUAL(static_cast<int>(GestureDirection::None),
                      static_cast<int>(GestureNavigation::classify(-100, 10, 1000U)));
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
    RUN_TEST(test_swipe_left_follows_screen_ring);
    RUN_TEST(test_swipe_right_follows_reverse_screen_ring);
    RUN_TEST(test_classifier_accepts_clear_horizontal_swipes);
    RUN_TEST(test_classifier_rejects_short_vertical_and_long_press_movements);
    RUN_TEST(test_vertical_or_none_does_not_change_page);
    return UNITY_END();
}
