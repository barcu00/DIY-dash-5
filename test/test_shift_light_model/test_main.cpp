#include <cstddef>

#include <unity.h>

// Production changes caught: DASH and TRACK computing different LEDs, wrong
// start/max clamping, or the red zone beginning at the wrong segment.

#include "ui/shift_light_model.h"

namespace {
std::size_t litCount(const ShiftSegmentStates& states) {
    std::size_t count = 0U;
    for (const ShiftSegmentState& state : states) {
        if (state.lit) {
            ++count;
        }
    }
    return count;
}
}  // namespace

void test_rpm_below_start_keeps_every_segment_off() {
    const ShiftLightConfig config{5500U, 7000U, 7500U, 8000U, true};
    const ShiftSegmentStates states =
        ShiftLightModel::segments(5499U, true, 0U, config);

    TEST_ASSERT_EQUAL_UINT32(0U, litCount(states));
    for (const ShiftSegmentState& state : states) {
        TEST_ASSERT_FALSE(state.lit);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Off),
                                static_cast<uint8_t>(state.color));
    }
}

void test_start_lights_first_segment_and_max_lights_all_segments() {
    ShiftLightConfig config{5500U, 7000U, 7500U, 8000U, false};

    TEST_ASSERT_EQUAL_UINT32(
        1U, litCount(ShiftLightModel::segments(5500U, true, 0U, config)));
    TEST_ASSERT_EQUAL_UINT32(
        12U, litCount(ShiftLightModel::segments(8000U, true, 0U, config)));
    TEST_ASSERT_EQUAL_UINT32(
        12U, litCount(ShiftLightModel::segments(9000U, true, 0U, config)));
}

void test_progressive_strip_uses_fixed_color_quarters() {
    const ShiftLightConfig config{5500U, 7000U, 7500U, 8000U, true};
    const ShiftSegmentStates states =
        ShiftLightModel::segments(7250U, true, 0U, config);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Yellow),
                            static_cast<uint8_t>(states[4].color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Yellow),
                            static_cast<uint8_t>(states[5].color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Yellow),
                            static_cast<uint8_t>(states[7].color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Red),
                            static_cast<uint8_t>(states[8].color));
}

void test_same_input_produces_identical_dash_and_track_states() {
    const ShiftLightConfig shared_config{5500U, 7000U, 7500U, 8000U, true};
    const ShiftSegmentStates dash =
        ShiftLightModel::segments(7250U, true, 0U, shared_config);
    const ShiftSegmentStates track =
        ShiftLightModel::segments(7250U, true, 0U, shared_config);

    for (std::size_t i = 0U; i < dash.size(); ++i) {
        TEST_ASSERT_EQUAL_UINT8(dash[i].lit, track[i].lit);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(dash[i].color),
                                static_cast<uint8_t>(track[i].color));
    }
}

void test_invalid_configuration_fails_safe_with_all_segments_off() {
    const ShiftSegmentStates states = ShiftLightModel::segments(
        7500U, true, 0U,
        ShiftLightConfig{8000U, 7000U, 7500U, 6000U, true});

    TEST_ASSERT_EQUAL_UINT32(0U, litCount(states));
}

void test_enabled_flash_alternates_the_full_strip_red_and_off() {
    const ShiftLightConfig config{5500U, 7000U, 7500U, 8000U, true};
    const ShiftSegmentStates red =
        ShiftLightModel::segments(7500U, true, 0U, config);
    const ShiftSegmentStates red_end =
        ShiftLightModel::segments(7500U, true, 124U, config);
    const ShiftSegmentStates off =
        ShiftLightModel::segments(7500U, true, 125U, config);
    const ShiftSegmentStates off_end =
        ShiftLightModel::segments(7500U, true, 249U, config);
    const ShiftSegmentStates red_again =
        ShiftLightModel::segments(7500U, true, 250U, config);

    TEST_ASSERT_EQUAL_UINT32(12U, litCount(red));
    TEST_ASSERT_EQUAL_UINT32(12U, litCount(red_end));
    TEST_ASSERT_EQUAL_UINT32(0U, litCount(off));
    TEST_ASSERT_EQUAL_UINT32(0U, litCount(off_end));
    TEST_ASSERT_EQUAL_UINT32(12U, litCount(red_again));
    for (const ShiftSegmentState& segment : red) {
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Red),
                                static_cast<uint8_t>(segment.color));
    }
}

void test_invalid_rpm_never_activates_flash() {
    const ShiftLightConfig config{5500U, 7000U, 7500U, 8000U, true};
    const ShiftSegmentStates states =
        ShiftLightModel::segments(9000U, false, 0U, config);

    TEST_ASSERT_EQUAL_UINT32(0U, litCount(states));
}

void test_progressive_segments_use_four_fixed_positions_per_color() {
    const ShiftLightConfig config{0U, 6000U, 9000U, 10000U, false};
    const ShiftSegmentStates states =
        ShiftLightModel::segments(10000U, true, 0U, config);

    for (std::size_t i = 0U; i < states.size(); ++i) {
        const ShiftColor expected = i < 4U ? ShiftColor::Green
                                  : i < 8U ? ShiftColor::Yellow
                                           : ShiftColor::Red;
        TEST_ASSERT_TRUE(states[i].lit);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected),
                                static_cast<uint8_t>(states[i].color));
    }
}

void test_zero_rpm_start_is_valid_and_lights_first_segment() {
    const ShiftLightConfig config{0U, 6000U, 9000U, 10000U, false};
    TEST_ASSERT_EQUAL_UINT32(
        1U, litCount(ShiftLightModel::segments(0U, true, 0U, config)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_rpm_below_start_keeps_every_segment_off);
    RUN_TEST(test_start_lights_first_segment_and_max_lights_all_segments);
    RUN_TEST(test_progressive_strip_uses_fixed_color_quarters);
    RUN_TEST(test_same_input_produces_identical_dash_and_track_states);
    RUN_TEST(test_invalid_configuration_fails_safe_with_all_segments_off);
    RUN_TEST(test_enabled_flash_alternates_the_full_strip_red_and_off);
    RUN_TEST(test_invalid_rpm_never_activates_flash);
    RUN_TEST(test_progressive_segments_use_four_fixed_positions_per_color);
    RUN_TEST(test_zero_rpm_start_is_valid_and_lights_first_segment);
    return UNITY_END();
}
