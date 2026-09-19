#include <cstddef>
#include <cstdint>

#include <unity.h>

#include "racechrono/racechrono_event_queue.h"

void test_queue_preserves_fifo_and_rejects_newest_on_overflow() {
    RaceChronoEventQueue<32U> queue;

    for (uint8_t index = 0U; index < 31U; ++index) {
        RaceChronoEvent event;
        event.type = RaceChronoEventType::ValuesWrite;
        event.size = 3U;
        event.bytes[0] = index;
        event.bytes[1] = static_cast<uint8_t>(index + 1U);
        event.bytes[2] = static_cast<uint8_t>(index + 2U);
        TEST_ASSERT_TRUE(queue.pushFromProducer(event));
    }

    RaceChronoEvent rejected;
    rejected.type = RaceChronoEventType::ConfigWrite;
    rejected.size = 1U;
    rejected.bytes[0] = 0xEEU;
    TEST_ASSERT_FALSE(queue.pushFromProducer(rejected));
    TEST_ASSERT_EQUAL_UINT32(1U, queue.droppedCount());

    for (uint8_t index = 0U; index < 31U; ++index) {
        RaceChronoEvent event;
        TEST_ASSERT_TRUE(queue.popFromConsumer(event));
        TEST_ASSERT_EQUAL_UINT8(
            static_cast<uint8_t>(RaceChronoEventType::ValuesWrite),
            static_cast<uint8_t>(event.type));
        TEST_ASSERT_EQUAL_UINT8(3U, event.size);
        TEST_ASSERT_EQUAL_UINT8(index, event.bytes[0]);
        TEST_ASSERT_EQUAL_UINT8(index + 1U, event.bytes[1]);
        TEST_ASSERT_EQUAL_UINT8(index + 2U, event.bytes[2]);
    }

    RaceChronoEvent empty;
    TEST_ASSERT_FALSE(queue.popFromConsumer(empty));
}

void test_clear_discards_pending_events_without_resetting_drop_diagnostics() {
    RaceChronoEventQueue<4U> queue;
    RaceChronoEvent event;
    TEST_ASSERT_TRUE(queue.pushFromProducer(event));
    TEST_ASSERT_TRUE(queue.pushFromProducer(event));
    TEST_ASSERT_TRUE(queue.pushFromProducer(event));
    TEST_ASSERT_FALSE(queue.pushFromProducer(event));

    queue.clear();

    TEST_ASSERT_FALSE(queue.popFromConsumer(event));
    TEST_ASSERT_EQUAL_UINT32(1U, queue.droppedCount());
    TEST_ASSERT_TRUE(queue.pushFromProducer(event));
    TEST_ASSERT_TRUE(queue.popFromConsumer(event));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_queue_preserves_fifo_and_rejects_newest_on_overflow);
    RUN_TEST(test_clear_discards_pending_events_without_resetting_drop_diagnostics);
    return UNITY_END();
}
