#include <unity.h>

#include "ecu/ecu_can_decoder.h"
#include "telemetry/telemetry_manager.h"

namespace {
const SignalDefinition kDefinitions[] = {
    {0x321U, false, 0U, ByteOrder::Little, RawType::Unsigned16,
     1.0f, 0.0f, VehicleSignal::Rpm, "rpm", 250U},
};
}

void test_valid_decoded_frame_selects_can_snapshot() {
    EcuCanDecoder decoder(kDefinitions, 1U);
    TelemetryManager manager(decoder, 500U);
    manager.selectSource(DataSource::Can, 0U);
    const CanFrame frame{0x321U, 2U, {0x68U, 0x10U}, false, false};

    TEST_ASSERT_TRUE(manager.accept(frame, 100U));
    manager.update(150U);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanStatus::Online),
                            static_cast<uint8_t>(manager.canStatus()));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Can),
                            static_cast<uint8_t>(manager.state().source()));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4200.0f,
                             manager.state().get(VehicleSignal::Rpm).value);
}

void test_can_timeout_never_falls_back_to_demo() {
    EcuCanDecoder decoder(kDefinitions, 1U);
    TelemetryManager manager(decoder, 500U);
    manager.selectSource(DataSource::Can, 0U);
    const CanFrame frame{0x321U, 2U, {0x68U, 0x10U}, false, false};
    manager.accept(frame, 100U);

    manager.update(601U);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanStatus::Offline),
                            static_cast<uint8_t>(manager.canStatus()));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::None),
                            static_cast<uint8_t>(manager.state().source()));
    TEST_ASSERT_FALSE(manager.demoActive());
    TEST_ASSERT_FALSE(manager.state().get(VehicleSignal::Rpm).valid);
}

void test_explicit_demo_selection_ignores_can_frames() {
    EcuCanDecoder decoder(kDefinitions, 1U);
    TelemetryManager manager(decoder, 500U);
    manager.selectSource(DataSource::Demo, 0U);
    const CanFrame frame{0x321U, 2U, {0x68U, 0x10U}, false, false};

    TEST_ASSERT_FALSE(manager.accept(frame, 100U));
    manager.update(100U);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(manager.state().source()));
    TEST_ASSERT_TRUE(manager.state().get(VehicleSignal::Rpm).valid);
    TEST_ASSERT_TRUE(manager.demoActive());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanStatus::Disabled),
                            static_cast<uint8_t>(manager.canStatus()));
}

void test_unmapped_frame_never_marks_can_online() {
    EcuCanDecoder decoder(kDefinitions, 1U);
    TelemetryManager manager(decoder, 500U);
    manager.selectSource(DataSource::Can, 0U);
    const CanFrame frame{0x777U, 2U, {0x68U, 0x10U}, false, false};

    TEST_ASSERT_FALSE(manager.accept(frame, 100U));
    manager.update(200U);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanStatus::Waiting),
                            static_cast<uint8_t>(manager.canStatus()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_decoded_frame_selects_can_snapshot);
    RUN_TEST(test_can_timeout_never_falls_back_to_demo);
    RUN_TEST(test_explicit_demo_selection_ignores_can_frames);
    RUN_TEST(test_unmapped_frame_never_marks_can_online);
    return UNITY_END();
}
