#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include <unity.h>

#include "racechrono/racechrono_runtime.h"

namespace {

enum class TransportCall : uint8_t {
    StartAdvertising,
    StopAdvertising,
    Disconnect,
    Indicate,
};

class FakeTransport final : public RaceChronoTransport {
public:
    bool startAdvertising() override {
        calls.push_back(TransportCall::StartAdvertising);
        return true;
    }
    void stopAdvertising() override {
        calls.push_back(TransportCall::StopAdvertising);
    }
    void disconnect() override {
        calls.push_back(TransportCall::Disconnect);
    }
    bool indicate(const uint8_t* data, std::size_t size) override {
        calls.push_back(TransportCall::Indicate);
        if (fail_next_indication) {
            fail_next_indication = false;
            return false;
        }
        RaceChronoPacket packet;
        packet.size = static_cast<uint8_t>(size);
        std::memcpy(packet.bytes.data(), data, size);
        indications.push_back(packet);
        return true;
    }

    std::vector<TransportCall> calls;
    std::vector<RaceChronoPacket> indications;
    bool fail_next_indication = false;
};

RaceChronoEvent event(RaceChronoEventType type) {
    RaceChronoEvent result;
    result.type = type;
    return result;
}

RaceChronoEvent configSuccess(uint8_t monitor_id) {
    RaceChronoEvent result = event(RaceChronoEventType::ConfigWrite);
    result.size = 2U;
    result.bytes[0] = 0U;
    result.bytes[1] = monitor_id;
    return result;
}

void enqueueAndService(RaceChronoRuntime& runtime,
                       const RaceChronoEvent& next,
                       uint32_t now_ms = 0U) {
    TEST_ASSERT_TRUE(runtime.enqueueEvent(next));
    runtime.service(now_ms);
}

void enableAndSubscribe(RaceChronoRuntime& runtime,
                        FakeTransport& transport) {
    runtime.setEnabled(true, 0U);
    runtime.service(0U);
    TEST_ASSERT_EQUAL_UINT32(1U, transport.calls.size());
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(TransportCall::StartAdvertising),
        static_cast<uint8_t>(transport.calls[0]));
    enqueueAndService(runtime, event(RaceChronoEventType::Connected));
    enqueueAndService(runtime,
                      event(RaceChronoEventType::IndicationsSubscribed));
}

}  // namespace

void test_enable_and_complete_handshake_emit_remove_add_update_order() {
    FakeTransport transport;
    RaceChronoRuntime runtime(transport);
    enableAndSubscribe(runtime, transport);

    TEST_ASSERT_EQUAL_UINT32(1U, transport.indications.size());
    TEST_ASSERT_EQUAL_UINT8(0U, transport.indications.front().bytes[0]);
    enqueueAndService(runtime,
                      event(RaceChronoEventType::IndicationConfirmed));

    for (uint8_t monitor_id = 1U; monitor_id <= 33U; ++monitor_id) {
        while (transport.indications.back().bytes[0] == 2U) {
            TEST_ASSERT_EQUAL_UINT8(
                monitor_id, transport.indications.back().bytes[1]);
            enqueueAndService(runtime,
                              event(RaceChronoEventType::IndicationConfirmed));
        }
        TEST_ASSERT_EQUAL_UINT8(3U,
                               transport.indications.back().bytes[0]);
        TEST_ASSERT_EQUAL_UINT8(monitor_id,
                               transport.indications.back().bytes[1]);
        enqueueAndService(runtime,
                          event(RaceChronoEventType::IndicationConfirmed));
        enqueueAndService(runtime, configSuccess(monitor_id));
    }

    TEST_ASSERT_EQUAL_UINT8(4U, transport.indications.back().bytes[0]);
    for (std::size_t index = 1U;
         index + 1U < transport.indications.size(); ++index) {
        TEST_ASSERT_TRUE(transport.indications[index].bytes[0] == 2U ||
                         transport.indications[index].bytes[0] == 3U);
    }
}

void test_service_consumes_at_most_eight_events() {
    FakeTransport transport;
    RaceChronoRuntime runtime(transport);
    runtime.setEnabled(true, 0U);
    runtime.service(0U);

    for (int index = 0; index < 8; ++index)
        TEST_ASSERT_TRUE(runtime.enqueueEvent(
            event(RaceChronoEventType::Connected)));
    TEST_ASSERT_TRUE(runtime.enqueueEvent(
        event(RaceChronoEventType::IndicationsSubscribed)));

    runtime.service(1U);
    TEST_ASSERT_EQUAL_UINT32(0U, transport.indications.size());
    runtime.service(2U);
    TEST_ASSERT_EQUAL_UINT32(1U, transport.indications.size());
    TEST_ASSERT_EQUAL_UINT8(0U, transport.indications[0].bytes[0]);
}

void test_restart_disconnects_then_advertises() {
    FakeTransport transport;
    RaceChronoRuntime runtime(transport);
    runtime.setEnabled(true, 0U);
    runtime.service(0U);
    transport.calls.clear();

    runtime.restart(10U);

    TEST_ASSERT_EQUAL_UINT32(2U, transport.calls.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransportCall::Disconnect),
                            static_cast<uint8_t>(transport.calls[0]));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(TransportCall::StartAdvertising),
        static_cast<uint8_t>(transport.calls[1]));
}

void test_disable_invalidates_racechrono_without_touching_engine_state() {
    FakeTransport transport;
    RaceChronoRuntime runtime(transport);
    runtime.setEnabled(true, 0U);
    runtime.service(0U);
    TEST_ASSERT_TRUE(runtime.telemetry().acceptRaw(17U, 1234, 20U));

    VehicleState engine;
    engine.reset(DataSource::Can);
    engine.set(ParameterId::Rpm, 6840.0f, 20U);
    engine.set(ParameterId::OilTemperature, 108.0f, 20U);
    const VehicleState before = engine;

    runtime.setEnabled(false, 30U);
    runtime.service(30U);

    TEST_ASSERT_FALSE(runtime.telemetry().get(ParameterId::RcGpsSpeed).valid);
    TEST_ASSERT_EQUAL_MEMORY(&before, &engine, sizeof(engine));
}

void test_failed_indication_is_retried_without_stalling_configuration() {
    FakeTransport transport;
    RaceChronoRuntime runtime(transport);
    runtime.setEnabled(true, 0U);
    runtime.service(0U);
    enqueueAndService(runtime, event(RaceChronoEventType::Connected));
    transport.fail_next_indication = true;

    enqueueAndService(runtime,
                      event(RaceChronoEventType::IndicationsSubscribed));
    TEST_ASSERT_EQUAL_UINT32(0U, transport.indications.size());

    runtime.service(1U);
    TEST_ASSERT_EQUAL_UINT32(1U, transport.indications.size());
    TEST_ASSERT_EQUAL_UINT8(0U, transport.indications[0].bytes[0]);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_enable_and_complete_handshake_emit_remove_add_update_order);
    RUN_TEST(test_service_consumes_at_most_eight_events);
    RUN_TEST(test_restart_disconnects_then_advertises);
    RUN_TEST(test_disable_invalidates_racechrono_without_touching_engine_state);
    RUN_TEST(test_failed_indication_is_retried_without_stalling_configuration);
    return UNITY_END();
}
