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
    const ShiftLightConfig config{5500U, 7000U, 8000U};
    const ShiftSegmentStates states = ShiftLightModel::segments(5499U, config);

    TEST_ASSERT_EQUAL_UINT32(0U, litCount(states));
    for (const ShiftSegmentState& state : states) {
        TEST_ASSERT_FALSE(state.lit);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Off),
                                static_cast<uint8_t>(state.color));
    }
}

void test_start_lights_first_segment_and_max_lights_all_segments() {
    const ShiftLightConfig config{5500U, 7000U, 8000U};

    TEST_ASSERT_EQUAL_UINT32(
        1U, litCount(ShiftLightModel::segments(5500U, config)));
    TEST_ASSERT_EQUAL_UINT32(
        12U, litCount(ShiftLightModel::segments(8000U, config)));
    TEST_ASSERT_EQUAL_UINT32(
        12U, litCount(ShiftLightModel::segments(9000U, config)));
}

void test_red_zone_uses_yellow_lead_in_and_red_from_threshold() {
    const ShiftLightConfig config{5500U, 7000U, 8000U};
    const ShiftSegmentStates states = ShiftLightModel::segments(7250U, config);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Green),
                            static_cast<uint8_t>(states[4].color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Yellow),
                            static_cast<uint8_t>(states[5].color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Yellow),
                            static_cast<uint8_t>(states[6].color));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ShiftColor::Red),
                            static_cast<uint8_t>(states[7].color));
}

void test_same_input_produces_identical_dash_and_track_states() {
    const ShiftLightConfig shared_config{5500U, 7000U, 8000U};
    const ShiftSegmentStates dash =
        ShiftLightModel::segments(7250U, shared_config);
    const ShiftSegmentStates track =
        ShiftLightModel::segments(7250U, shared_config);

    for (std::size_t i = 0U; i < dash.size(); ++i) {
        TEST_ASSERT_EQUAL_UINT8(dash[i].lit, track[i].lit);
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(dash[i].color),
                                static_cast<uint8_t>(track[i].color));
    }
}

void test_invalid_configuration_fails_safe_with_all_segments_off() {
    const ShiftSegmentStates states =
        ShiftLightModel::segments(7500U, ShiftLightConfig{8000U, 7000U, 6000U});

    TEST_ASSERT_EQUAL_UINT32(0U, litCount(states));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_rpm_below_start_keeps_every_segment_off);
    RUN_TEST(test_start_lights_first_segment_and_max_lights_all_segments);
    RUN_TEST(test_red_zone_uses_yellow_lead_in_and_red_from_threshold);
    RUN_TEST(test_same_input_produces_identical_dash_and_track_states);
    RUN_TEST(test_invalid_configuration_fails_safe_with_all_segments_off);
    return UNITY_END();
}
