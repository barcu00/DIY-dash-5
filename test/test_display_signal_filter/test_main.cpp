#include <unity.h>

#include "ui/display_signal_filter.h"

namespace {
SignalValue signal(float value, uint32_t updated_ms) {
    return SignalValue{value, updated_ms, true};
}
}

void test_first_valid_value_is_shown_without_startup_lag() {
    DisplaySignalFilter filter;
    const SignalValue shown = filter.sample(signal(1000.0f, 10U), 10U, 50U);

    TEST_ASSERT_TRUE(shown.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, shown.value);
}

void test_changed_value_moves_smoothly_and_reaches_target() {
    DisplaySignalFilter filter;
    filter.sample(signal(1000.0f, 10U), 10U, 50U);

    const SignalValue first = filter.sample(signal(2000.0f, 20U), 20U, 50U);
    const SignalValue middle = filter.sample(signal(2000.0f, 20U), 45U, 50U);
    const SignalValue end = filter.sample(signal(2000.0f, 20U), 70U, 50U);

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, first.value);
    TEST_ASSERT_TRUE(middle.value > 1000.0f && middle.value < 2000.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2000.0f, end.value);
}

void test_invalid_input_resets_immediately() {
    DisplaySignalFilter filter;
    filter.sample(signal(1000.0f, 10U), 10U, 50U);

    const SignalValue invalid = filter.sample(SignalValue{}, 20U, 50U);
    const SignalValue recovered = filter.sample(signal(1500.0f, 30U), 30U, 50U);

    TEST_ASSERT_FALSE(invalid.valid);
    TEST_ASSERT_TRUE(recovered.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1500.0f, recovered.value);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_first_valid_value_is_shown_without_startup_lag);
    RUN_TEST(test_changed_value_moves_smoothly_and_reaches_target);
    RUN_TEST(test_invalid_input_resets_immediately);
    return UNITY_END();
}
