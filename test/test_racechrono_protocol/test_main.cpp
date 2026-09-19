#include <array>
#include <cstdint>
#include <cstring>

#include <unity.h>

#include "racechrono/racechrono_protocol.h"

void test_single_byte_commands_are_encoded_exactly() {
    const RaceChronoPacket remove = RaceChronoProtocol::removeAll();
    TEST_ASSERT_EQUAL_UINT8(1U, remove.size);
    TEST_ASSERT_EQUAL_HEX8(0x00U, remove.bytes[0]);

    const RaceChronoPacket update = RaceChronoProtocol::updateAll();
    TEST_ASSERT_EQUAL_UINT8(1U, update.size);
    TEST_ASSERT_EQUAL_HEX8(0x04U, update.bytes[0]);
}

void test_equations_are_fragmented_into_seventeen_byte_payloads() {
    constexpr char equation[] = "0123456789abcdefghijklmnopqrstuvwxy";
    TEST_ASSERT_EQUAL_UINT32(3U,
                             RaceChronoProtocol::fragmentCount(equation));

    const RaceChronoPacket first =
        RaceChronoProtocol::addFragment(29U, equation, 0U);
    TEST_ASSERT_EQUAL_UINT8(20U, first.size);
    TEST_ASSERT_EQUAL_HEX8(0x02U, first.bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(29U, first.bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(0U, first.bytes[2]);
    TEST_ASSERT_EQUAL_MEMORY(equation, first.bytes.data() + 3U, 17U);

    const RaceChronoPacket preceding =
        RaceChronoProtocol::addFragment(29U, equation, 1U);
    TEST_ASSERT_EQUAL_UINT8(20U, preceding.size);
    TEST_ASSERT_EQUAL_HEX8(0x02U, preceding.bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(1U, preceding.bytes[2]);

    const RaceChronoPacket final =
        RaceChronoProtocol::addFragment(29U, equation, 2U);
    TEST_ASSERT_EQUAL_HEX8(0x03U, final.bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(2U, final.bytes[2]);
    TEST_ASSERT_EQUAL_UINT8(4U, final.size);
    TEST_ASSERT_EQUAL_CHAR('y', final.bytes[3]);

    TEST_ASSERT_EQUAL_UINT8(
        0U, RaceChronoProtocol::addFragment(29U, equation, 3U).size);
    TEST_ASSERT_EQUAL_UINT8(
        0U, RaceChronoProtocol::addFragment(29U, nullptr, 0U).size);
}

void test_values_decode_four_big_endian_signed_records() {
    constexpr std::array<uint8_t, 20U> payload{{
        17U, 0x00U, 0x00U, 0x04U, 0xD2U,
        10U, 0xFFU, 0xFFU, 0xFEU, 0xC0U,
        22U, 0x00U, 0x00U, 0x00U, 0x0CU,
        30U, 0x00U, 0x00U, 0x03U, 0xE8U,
    }};

    const RaceChronoValueBatch batch =
        RaceChronoProtocol::decodeValues(payload.data(), payload.size());
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoDecodeError::None),
        static_cast<uint8_t>(batch.error));
    TEST_ASSERT_EQUAL_UINT8(4U, batch.count);
    TEST_ASSERT_EQUAL_UINT8(17U, batch.values[0].monitor_id);
    TEST_ASSERT_EQUAL_INT32(1234, batch.values[0].raw);
    TEST_ASSERT_EQUAL_UINT8(10U, batch.values[1].monitor_id);
    TEST_ASSERT_EQUAL_INT32(-320, batch.values[1].raw);
    TEST_ASSERT_EQUAL_INT32(12, batch.values[2].raw);
    TEST_ASSERT_EQUAL_INT32(1000, batch.values[3].raw);
}

void test_invalid_values_write_is_rejected_atomically() {
    std::array<uint8_t, 19U> malformed{};
    malformed[0] = 17U;
    malformed[4] = 1U;

    const RaceChronoValueBatch batch =
        RaceChronoProtocol::decodeValues(malformed.data(), malformed.size());
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoDecodeError::InvalidLength),
        static_cast<uint8_t>(batch.error));
    TEST_ASSERT_EQUAL_UINT8(0U, batch.count);

    const RaceChronoValueBatch empty =
        RaceChronoProtocol::decodeValues(nullptr, 0U);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoDecodeError::Empty),
        static_cast<uint8_t>(empty.error));
    TEST_ASSERT_EQUAL_UINT8(0U, empty.count);
}

void test_configuration_results_decode_exact_big_endian_fields() {
    constexpr uint8_t success_bytes[]{0U, 17U};
    const RaceChronoConfigResult success =
        RaceChronoProtocol::decodeConfigResult(success_bytes,
                                                sizeof(success_bytes));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConfigResultType::Success),
        static_cast<uint8_t>(success.type));
    TEST_ASSERT_EQUAL_UINT8(17U, success.monitor_id);

    constexpr uint8_t sequence_bytes[]{1U, 8U};
    const RaceChronoConfigResult sequence =
        RaceChronoProtocol::decodeConfigResult(sequence_bytes,
                                                sizeof(sequence_bytes));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConfigResultType::PayloadOutOfSequence),
        static_cast<uint8_t>(sequence.type));
    TEST_ASSERT_EQUAL_UINT8(8U, sequence.monitor_id);

    constexpr uint8_t exception_bytes[]{
        2U, 9U, 0x00U, 0x08U, 0x01U, 0x23U, 0x00U, 0x07U};
    const RaceChronoConfigResult exception =
        RaceChronoProtocol::decodeConfigResult(exception_bytes,
                                                sizeof(exception_bytes));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RaceChronoConfigResultType::EquationException),
        static_cast<uint8_t>(exception.type));
    TEST_ASSERT_EQUAL_UINT8(9U, exception.monitor_id);
    TEST_ASSERT_EQUAL_UINT16(8U, exception.exception_type);
    TEST_ASSERT_EQUAL_UINT16(0x0123U, exception.exception_position);
    TEST_ASSERT_EQUAL_UINT16(7U, exception.exception_length);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_single_byte_commands_are_encoded_exactly);
    RUN_TEST(test_equations_are_fragmented_into_seventeen_byte_payloads);
    RUN_TEST(test_values_decode_four_big_endian_signed_records);
    RUN_TEST(test_invalid_values_write_is_rejected_atomically);
    RUN_TEST(test_configuration_results_decode_exact_big_endian_fields);
    return UNITY_END();
}
