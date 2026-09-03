#include <array>

#include <unity.h>

#include "telemetry/vehicle_state.h"

void test_new_state_exposes_invalid_signals() {
    VehicleState state;

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::None),
                            static_cast<uint8_t>(state.source()));
    TEST_ASSERT_FALSE(state.get(VehicleSignal::Rpm).valid);
}

void test_source_reset_prevents_mixed_snapshots() {
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(VehicleSignal::Rpm, 4200.0f, 100U);

    state.reset(DataSource::Demo);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(state.source()));
    TEST_ASSERT_FALSE(state.get(VehicleSignal::Rpm).valid);
}

void test_stale_signal_is_invalidated_at_declared_timeout() {
    VehicleState state;
    state.reset(DataSource::Can);
    state.set(VehicleSignal::Rpm, 4200.0f, 100U);
    std::array<uint32_t, VehicleState::kSignalCount> timeouts{};
    timeouts[static_cast<std::size_t>(VehicleSignal::Rpm)] = 250U;

    state.invalidateStale(350U, timeouts.data());
    TEST_ASSERT_TRUE(state.get(VehicleSignal::Rpm).valid);

    state.invalidateStale(351U, timeouts.data());
    TEST_ASSERT_FALSE(state.get(VehicleSignal::Rpm).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_new_state_exposes_invalid_signals);
    RUN_TEST(test_source_reset_prevents_mixed_snapshots);
    RUN_TEST(test_stale_signal_is_invalidated_at_declared_timeout);
    return UNITY_END();
}
