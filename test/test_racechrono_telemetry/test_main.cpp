#include <array>
#include <cstdint>
#include <limits>

#include <unity.h>

#include "racechrono/racechrono_telemetry.h"

void test_raw_values_use_catalog_scale_and_signed_decoding() {
    RaceChronoTelemetry telemetry;

    TEST_ASSERT_TRUE(telemetry.acceptRaw(10U, -320, 1000U));
    const SignalValue& delta = telemetry.get(ParameterId::RcDeltaLapTime);
    TEST_ASSERT_TRUE(delta.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -0.320f, delta.value);
    TEST_ASSERT_EQUAL_UINT32(1000U, delta.updated_ms);

    TEST_ASSERT_TRUE(telemetry.acceptRaw(20U, 313200000, 1100U));
    const SignalValue& latitude = telemetry.get(ParameterId::RcLatitude);
    TEST_ASSERT_TRUE(latitude.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 52.2f, latitude.value);
}

void test_coordinate_invalid_sentinel_clears_only_that_channel() {
    RaceChronoTelemetry telemetry;
    TEST_ASSERT_TRUE(telemetry.acceptRaw(20U, 313200000, 1000U));
    TEST_ASSERT_TRUE(telemetry.acceptRaw(10U, -320, 1000U));

    TEST_ASSERT_FALSE(telemetry.acceptRaw(
        20U, std::numeric_limits<int32_t>::max(), 1200U));

    TEST_ASSERT_FALSE(telemetry.get(ParameterId::RcLatitude).valid);
    TEST_ASSERT_TRUE(telemetry.get(ParameterId::RcDeltaLapTime).valid);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoChannelState::NoData),
        static_cast<uint8_t>(
            telemetry.channelState(ParameterId::RcLatitude)));
}

void test_invalid_sentinel_is_never_scaled_into_a_numeric_channel() {
    RaceChronoTelemetry telemetry;
    TEST_ASSERT_TRUE(telemetry.acceptRaw(3U, 1234, 1000U));

    TEST_ASSERT_FALSE(telemetry.acceptRaw(
        3U, std::numeric_limits<int32_t>::max(), 1200U));

    TEST_ASSERT_FALSE(telemetry.get(ParameterId::RcLapDistance).valid);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoChannelState::NoData),
        static_cast<uint8_t>(
            telemetry.channelState(ParameterId::RcLapDistance)));
}

void test_unknown_monitor_ids_are_counted_without_publishing() {
    RaceChronoTelemetry telemetry;
    TEST_ASSERT_FALSE(telemetry.acceptRaw(99U, 1234, 1000U));

    const RaceChronoRuntimeStatus status = telemetry.snapshotStatus(1000U);
    TEST_ASSERT_EQUAL_UINT32(1U, status.unknown_monitor_ids);
    TEST_ASSERT_EQUAL_UINT8(0U, status.active_channels);
    TEST_ASSERT_FALSE(telemetry.get(ParameterId::RcGpsSpeed).valid);
}

void test_malformed_batch_is_rejected_atomically() {
    RaceChronoTelemetry telemetry;
    RaceChronoValueBatch batch;
    batch.count = 1U;
    batch.values[0] = RaceChronoDecodedValue{17U, 1234};
    batch.error = RaceChronoDecodeError::InvalidLength;

    TEST_ASSERT_FALSE(telemetry.acceptBatch(batch, 1000U));

    const RaceChronoRuntimeStatus status = telemetry.snapshotStatus(1000U);
    TEST_ASSERT_EQUAL_UINT32(1U, status.malformed_packets);
    TEST_ASSERT_EQUAL_UINT32(0U, status.value_packets);
    TEST_ASSERT_FALSE(telemetry.get(ParameterId::RcGpsSpeed).valid);
}

void test_stale_update_and_wraparound_invalidate_only_expired_channels() {
    RaceChronoTelemetry telemetry;
    constexpr uint32_t start = UINT32_MAX - 1000U;
    TEST_ASSERT_TRUE(telemetry.acceptRaw(17U, 1234, start));
    TEST_ASSERT_TRUE(telemetry.acceptRaw(22U, 8, start + 500U));

    telemetry.updateStale(4000U);
    TEST_ASSERT_FALSE(telemetry.get(ParameterId::RcGpsSpeed).valid);
    TEST_ASSERT_TRUE(telemetry.get(ParameterId::RcSatellites).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_raw_values_use_catalog_scale_and_signed_decoding);
    RUN_TEST(test_coordinate_invalid_sentinel_clears_only_that_channel);
    RUN_TEST(test_invalid_sentinel_is_never_scaled_into_a_numeric_channel);
    RUN_TEST(test_unknown_monitor_ids_are_counted_without_publishing);
    RUN_TEST(test_malformed_batch_is_rejected_atomically);
    RUN_TEST(test_stale_update_and_wraparound_invalidate_only_expired_channels);
    return UNITY_END();
}
