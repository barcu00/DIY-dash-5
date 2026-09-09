#include <unity.h>

#include "can/can_frame.h"
#include "ecu/ecu_can_decoder.h"
#include "telemetry/vehicle_state.h"

void test_decoder_applies_little_endian_scale_and_bias() {
    const SignalDefinition definitions[] = {
        {0x321U, false, 1U, ByteOrder::Little, RawType::Unsigned16,
         0.5f, -10.0f, VehicleSignal::Rpm, "rpm", 250U},
    };
    EcuCanDecoder decoder(definitions, 1U);
    VehicleState state;
    state.reset(DataSource::Can);
    const CanFrame frame{0x321U, 3U, {0x00U, 0x34U, 0x12U}, false, false};

    TEST_ASSERT_TRUE(decoder.decode(frame, state, 50U));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2320.0f,
                             state.get(VehicleSignal::Rpm).value);
}

void test_decoder_sign_extends_big_endian_value() {
    const SignalDefinition definitions[] = {
        {0x222U, false, 0U, ByteOrder::Big, RawType::Signed16,
         0.1f, 0.0f, VehicleSignal::Clt, "C", 500U},
    };
    EcuCanDecoder decoder(definitions, 1U);
    VehicleState state;
    state.reset(DataSource::Can);
    const CanFrame frame{0x222U, 2U, {0xFFU, 0x9CU}, false, false};

    TEST_ASSERT_TRUE(decoder.decode(frame, state, 5U));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -10.0f,
                             state.get(VehicleSignal::Clt).value);
}

void test_decoder_supports_unsigned_32_bit_value() {
    const SignalDefinition definitions[] = {
        {0x123U, true, 0U, ByteOrder::Little, RawType::Unsigned32,
         0.001f, 0.0f, VehicleSignal::Speed, "km/h", 100U},
    };
    EcuCanDecoder decoder(definitions, 1U);
    VehicleState state;
    state.reset(DataSource::Can);
    const CanFrame frame{0x123U, 4U, {0xA0U, 0x86U, 0x01U, 0x00U}, true, false};

    TEST_ASSERT_TRUE(decoder.decode(frame, state, 6U));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f,
                             state.get(VehicleSignal::Speed).value);
}

void test_decoder_rejects_short_or_wrong_format_frames() {
    const SignalDefinition definitions[] = {
        {0x321U, false, 1U, ByteOrder::Little, RawType::Unsigned16,
         1.0f, 0.0f, VehicleSignal::Rpm, "rpm", 250U},
    };
    EcuCanDecoder decoder(definitions, 1U);
    VehicleState state;
    state.reset(DataSource::Can);
    const CanFrame short_frame{0x321U, 2U, {0x00U, 0x34U}, false, false};
    const CanFrame extended_frame{0x321U, 3U, {0x00U, 0x34U, 0x12U}, true, false};

    TEST_ASSERT_FALSE(decoder.decode(short_frame, state, 7U));
    TEST_ASSERT_FALSE(decoder.decode(extended_frame, state, 8U));
    TEST_ASSERT_FALSE(state.get(VehicleSignal::Rpm).valid);
}

void test_empty_profile_accepts_no_frame() {
    EcuCanDecoder decoder(nullptr, 0U);
    VehicleState state;
    state.reset(DataSource::Can);
    const CanFrame frame{0x600U, 8U, {0U}, false, false};

    TEST_ASSERT_FALSE(decoder.decode(frame, state, 10U));
    TEST_ASSERT_EQUAL_UINT32(0U, decoder.definitionCount());
}

void test_profile_decoder_extracts_masked_boolean_flags() {
    const CanSignalDefinition signals[] = {
        {ParameterId::CheckEngine, 0U, RawType::Unsigned8,
         ByteOrder::Little, 1.0f, 0.0f, 0.0f, 1.0f, 100U,
         CanSignalKind::MaskedFlag, 0x08U, 3U, 1ULL << 1U},
    };
    const CanFrameDefinition frames[] = {
        {0x555U, false, 1U, 0U, 0U, 0U, signals, 1U},
    };
    const CanProfile profile{
        "test_flags", "Test flags", "Test", "1",
        CanProfileVerification::Verified, 500000U, "source", "revision",
        "fixture", frames, 1U};
    EcuCanDecoder decoder(&profile);
    VehicleState state;
    state.reset(DataSource::Can);

    TEST_ASSERT_TRUE(decoder.decode(
        CanFrame{0x555U, 1U, {0x08U}, false, false}, state, 10U));
    TEST_ASSERT_EQUAL_FLOAT(1.0f,
                            state.get(ParameterId::CheckEngine).value);
    TEST_ASSERT_TRUE(decoder.decode(
        CanFrame{0x555U, 1U, {0x00U}, false, false}, state, 20U));
    TEST_ASSERT_EQUAL_FLOAT(0.0f,
                            state.get(ParameterId::CheckEngine).value);
}

void test_profile_decoder_matches_documented_multivalue_flag_states() {
    const CanSignalDefinition signals[] = {
        {ParameterId::AntiLagActive, 0U, RawType::Unsigned8,
         ByteOrder::Little, 1.0f, 0.0f, 0.0f, 1.0f, 100U,
         CanSignalKind::MaskedFlag, 0xE0U, 5U,
         (1ULL << 1U) | (1ULL << 4U) | (1ULL << 5U) | (1ULL << 6U)},
    };
    const CanFrameDefinition frames[] = {
        {0x556U, false, 1U, 0U, 0U, 0U, signals, 1U},
    };
    const CanProfile profile{
        "test_states", "Test states", "Test", "1",
        CanProfileVerification::Verified, 500000U, "source", "revision",
        "fixture", frames, 1U};
    EcuCanDecoder decoder(&profile);
    VehicleState state;
    state.reset(DataSource::Can);

    TEST_ASSERT_TRUE(decoder.decode(
        CanFrame{0x556U, 1U, {0x20U}, false, false}, state, 10U));
    TEST_ASSERT_EQUAL_FLOAT(1.0f,
                            state.get(ParameterId::AntiLagActive).value);
    TEST_ASSERT_TRUE(decoder.decode(
        CanFrame{0x556U, 1U, {0x40U}, false, false}, state, 20U));
    TEST_ASSERT_EQUAL_FLOAT(0.0f,
                            state.get(ParameterId::AntiLagActive).value);
}

void test_invalid_flag_definition_rejects_frame_transactionally() {
    const CanSignalDefinition signals[] = {
        {ParameterId::Rpm, 0U, RawType::Unsigned8,
         ByteOrder::Little, 100.0f, 0.0f, 0.0f, 10000.0f, 100U},
        {ParameterId::CheckEngine, 0U, RawType::Unsigned8,
         ByteOrder::Little, 1.0f, 0.0f, 0.0f, 1.0f, 100U,
         CanSignalKind::MaskedFlag, 0U, 0U, 1ULL << 1U},
    };
    const CanFrameDefinition frames[] = {
        {0x557U, false, 1U, 0U, 0U, 0U, signals, 2U},
    };
    const CanProfile profile{
        "invalid_flag", "Invalid flag", "Test", "1",
        CanProfileVerification::Verified, 500000U, "source", "revision",
        "fixture", frames, 1U};
    EcuCanDecoder decoder(&profile);
    VehicleState state;
    state.reset(DataSource::Can);

    TEST_ASSERT_FALSE(decoder.decode(
        CanFrame{0x557U, 1U, {50U}, false, false}, state, 10U));
    TEST_ASSERT_FALSE(state.get(ParameterId::Rpm).valid);
    TEST_ASSERT_FALSE(state.get(ParameterId::CheckEngine).valid);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_decoder_applies_little_endian_scale_and_bias);
    RUN_TEST(test_decoder_sign_extends_big_endian_value);
    RUN_TEST(test_decoder_supports_unsigned_32_bit_value);
    RUN_TEST(test_decoder_rejects_short_or_wrong_format_frames);
    RUN_TEST(test_empty_profile_accepts_no_frame);
    RUN_TEST(test_profile_decoder_extracts_masked_boolean_flags);
    RUN_TEST(test_profile_decoder_matches_documented_multivalue_flag_states);
    RUN_TEST(test_invalid_flag_definition_rejects_frame_transactionally);
    return UNITY_END();
}
