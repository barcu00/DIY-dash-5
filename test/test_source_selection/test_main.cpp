#include <unity.h>

#include "ecu/ecu_can_decoder.h"
#include "telemetry/telemetry_manager.h"

void test_source_can_and_demo_are_selected_explicitly() {
    EcuCanDecoder decoder(nullptr, 0U);
    TelemetryManager manager(decoder, 500U);

    manager.selectSource(DataSource::Demo, 10U);
    manager.update(20U);
    TEST_ASSERT_TRUE(manager.demoActive());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Demo),
                            static_cast<uint8_t>(manager.selectedSource()));

    manager.selectSource(DataSource::Can, 30U);
    manager.update(40U);
    TEST_ASSERT_FALSE(manager.demoActive());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::Can),
                            static_cast<uint8_t>(manager.selectedSource()));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DataSource::None),
                            static_cast<uint8_t>(manager.state().source()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_source_can_and_demo_are_selected_explicitly);
    return UNITY_END();
}
