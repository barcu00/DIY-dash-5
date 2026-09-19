#include <cstdint>

#include <unity.h>

#include "racechrono/racechrono_session.h"

namespace {
RaceChronoEvent event(RaceChronoEventType type) {
    RaceChronoEvent result;
    result.type = type;
    return result;
}

RaceChronoEvent configResult(uint8_t result, uint8_t monitor_id) {
    RaceChronoEvent output = event(RaceChronoEventType::ConfigWrite);
    output.size = 2U;
    output.bytes[0] = result;
    output.bytes[1] = monitor_id;
    return output;
}

RaceChronoEvent equationException(uint8_t monitor_id) {
    RaceChronoEvent output = event(RaceChronoEventType::ConfigWrite);
    output.size = 8U;
    output.bytes[0] = 2U;
    output.bytes[1] = monitor_id;
    output.bytes[3] = 8U;
    output.bytes[5] = 4U;
    output.bytes[7] = 2U;
    return output;
}

RaceChronoEvent gpsSpeedValue(int32_t raw) {
    RaceChronoEvent output = event(RaceChronoEventType::ValuesWrite);
    output.size = 5U;
    output.bytes[0] = 17U;
    const uint32_t encoded = static_cast<uint32_t>(raw);
    output.bytes[1] = static_cast<uint8_t>(encoded >> 24U);
    output.bytes[2] = static_cast<uint8_t>(encoded >> 16U);
    output.bytes[3] = static_cast<uint8_t>(encoded >> 8U);
    output.bytes[4] = static_cast<uint8_t>(encoded);
    return output;
}

RaceChronoAction take(RaceChronoSession& session) {
    RaceChronoAction action;
    session.takeAction(action);
    return action;
}

void beginConfiguration(RaceChronoSession& session) {
    session.setEnabled(true);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoActionType::StartAdvertising),
        static_cast<uint8_t>(take(session).type));
    session.onEvent(event(RaceChronoEventType::Connected), 0U);
    session.onEvent(event(RaceChronoEventType::IndicationsSubscribed), 0U);
    const RaceChronoAction remove = take(session);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoActionType::Indicate),
        static_cast<uint8_t>(remove.type));
    TEST_ASSERT_EQUAL_UINT8(0U, remove.packet.bytes[0]);
    session.onEvent(event(RaceChronoEventType::IndicationConfirmed), 0U);
}

RaceChronoAction advanceToCompleteFragment(RaceChronoSession& session,
                                           uint32_t now_ms) {
    RaceChronoAction action = take(session);
    while (action.packet.bytes[0] == 2U) {
        session.onEvent(event(RaceChronoEventType::IndicationConfirmed),
                        now_ms);
        action = take(session);
    }
    return action;
}

void finishAllChannels(RaceChronoSession& session, uint32_t now_ms) {
    for (uint8_t monitor_id = 1U; monitor_id <= 33U; ++monitor_id) {
        const RaceChronoAction final =
            advanceToCompleteFragment(session, now_ms);
        TEST_ASSERT_EQUAL_UINT8(3U, final.packet.bytes[0]);
        TEST_ASSERT_EQUAL_UINT8(monitor_id, final.packet.bytes[1]);
        session.onEvent(event(RaceChronoEventType::IndicationConfirmed),
                        now_ms);
        session.onEvent(configResult(0U, monitor_id), now_ms);
    }
    const RaceChronoAction update = take(session);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoActionType::Indicate),
        static_cast<uint8_t>(update.type));
    TEST_ASSERT_EQUAL_UINT8(4U, update.packet.bytes[0]);
    session.onEvent(event(RaceChronoEventType::IndicationConfirmed), now_ms);
}
}  // namespace

void test_complete_connection_configuration_and_no_data_graph() {
    RaceChronoTelemetry telemetry;
    RaceChronoSession session(telemetry);

    session.setEnabled(true);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Advertising),
        static_cast<uint8_t>(session.state()));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoActionType::StartAdvertising),
        static_cast<uint8_t>(take(session).type));

    session.onEvent(event(RaceChronoEventType::Connected), 0U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Connected),
        static_cast<uint8_t>(session.state()));
    session.onEvent(event(RaceChronoEventType::IndicationsSubscribed), 0U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Configuring),
        static_cast<uint8_t>(session.state()));
    const RaceChronoAction remove = take(session);
    TEST_ASSERT_EQUAL_UINT8(0U, remove.packet.bytes[0]);
    session.onEvent(event(RaceChronoEventType::IndicationConfirmed), 0U);

    finishAllChannels(session, 100U);
    session.onEvent(gpsSpeedValue(1234), 100U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Active),
        static_cast<uint8_t>(session.state()));
    TEST_ASSERT_FLOAT_WITHIN(
        0.001f, 12.34f, telemetry.get(ParameterId::RcGpsSpeed).value);

    session.update(2601U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::NoData),
        static_cast<uint8_t>(session.state()));
    session.onEvent(gpsSpeedValue(1300), 2602U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Active),
        static_cast<uint8_t>(session.state()));

    session.onEvent(event(RaceChronoEventType::Disconnected), 2700U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Advertising),
        static_cast<uint8_t>(session.state()));
}

void test_equation_exception_isolated_and_configuration_continues() {
    RaceChronoTelemetry telemetry;
    RaceChronoSession session(telemetry);
    beginConfiguration(session);

    RaceChronoAction final = advanceToCompleteFragment(session, 0U);
    TEST_ASSERT_EQUAL_UINT8(1U, final.packet.bytes[1]);
    session.onEvent(event(RaceChronoEventType::IndicationConfirmed), 0U);
    session.onEvent(configResult(0U, 1U), 0U);

    final = advanceToCompleteFragment(session, 0U);
    TEST_ASSERT_EQUAL_UINT8(2U, final.packet.bytes[1]);
    session.onEvent(event(RaceChronoEventType::IndicationConfirmed), 0U);
    session.onEvent(equationException(2U), 0U);

    const RaceChronoAction next = take(session);
    TEST_ASSERT_EQUAL_UINT8(3U, next.packet.bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoChannelState::Error),
        static_cast<uint8_t>(
            telemetry.channelState(ParameterId::RcLapTime)));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Configuring),
        static_cast<uint8_t>(session.state()));
}

void test_three_out_of_sequence_results_fail_only_current_channel() {
    RaceChronoTelemetry telemetry;
    RaceChronoSession session(telemetry);
    beginConfiguration(session);

    for (uint8_t attempt = 0U; attempt < 3U; ++attempt) {
        const RaceChronoAction final =
            advanceToCompleteFragment(session, 0U);
        TEST_ASSERT_EQUAL_UINT8(1U, final.packet.bytes[1]);
        session.onEvent(event(RaceChronoEventType::IndicationConfirmed), 0U);
        session.onEvent(configResult(1U, 1U), 0U);
    }

    const RaceChronoAction next = take(session);
    TEST_ASSERT_EQUAL_UINT8(2U, next.packet.bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoChannelState::Error),
        static_cast<uint8_t>(
            telemetry.channelState(ParameterId::RcLapNumber)));
}

void test_disable_clears_work_and_requests_disconnect_and_stop() {
    RaceChronoTelemetry telemetry;
    RaceChronoSession session(telemetry);
    session.setEnabled(true);
    (void)take(session);
    session.onEvent(event(RaceChronoEventType::Connected), 0U);

    session.setEnabled(false);

    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Disabled),
        static_cast<uint8_t>(session.state()));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoActionType::Disconnect),
        static_cast<uint8_t>(take(session).type));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoActionType::StopAdvertising),
        static_cast<uint8_t>(take(session).type));
}

void test_unsigned_deadlines_work_across_millis_rollover() {
    RaceChronoTelemetry telemetry;
    RaceChronoSession session(telemetry);
    beginConfiguration(session);
    constexpr uint32_t start = UINT32_MAX - 1000U;
    finishAllChannels(session, start);
    session.onEvent(gpsSpeedValue(1234), start);

    session.update(1000U);
    const RaceChronoAction refresh = take(session);
    TEST_ASSERT_EQUAL_UINT8(4U, refresh.packet.bytes[0]);
    session.onEvent(event(RaceChronoEventType::IndicationConfirmed), 1000U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::Active),
        static_cast<uint8_t>(session.state()));

    session.update(1501U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConnectionState::NoData),
        static_cast<uint8_t>(session.state()));
    session.update(4000U);
    TEST_ASSERT_FALSE(telemetry.get(ParameterId::RcGpsSpeed).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_complete_connection_configuration_and_no_data_graph);
    RUN_TEST(test_equation_exception_isolated_and_configuration_continues);
    RUN_TEST(test_three_out_of_sequence_results_fail_only_current_channel);
    RUN_TEST(test_disable_clears_work_and_requests_disconnect_and_stop);
    RUN_TEST(test_unsigned_deadlines_work_across_millis_rollover);
    return UNITY_END();
}
