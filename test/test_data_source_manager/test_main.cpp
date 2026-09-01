#include <unity.h>

#include "can/can_frame.h"
#include "telemetry/data_source_manager.h"

namespace {
CanFrame makeFrame(uint32_t id, std::initializer_list<uint8_t> bytes) {
    CanFrame frame{};
    frame.id = id;
    frame.dlc = static_cast<uint8_t>(bytes.size());
    std::size_t i = 0;
    for (uint8_t value : bytes) {
        if (i < 8) frame.data[i++] = value;
    }
    return frame;
}

void test_mock_source_publishes_registry_values() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);

    TEST_ASSERT_EQUAL(static_cast<int>(DataSource::Mock), static_cast<int>(manager.source()));
    manager.update(1000U);
    TEST_ASSERT_TRUE(registry.value(ParameterId::Rpm).valid);
    TEST_ASSERT_FALSE(manager.canOnline());
}

void test_ecumaster_valid_frame_sets_online_and_decodes() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);
    manager.setSource(DataSource::Ecumaster);

    const CanFrame frame = makeFrame(0x600, {0xE1,0x10, 100,20, 0xB9,0, 0x3E,0});
    TEST_ASSERT_TRUE(manager.handleCanFrame(frame, 2000U));
    TEST_ASSERT_TRUE(manager.canOnline());
    TEST_ASSERT_EQUAL_UINT32(1U, manager.receivedFrameCount());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 4321.0f, registry.value(ParameterId::Rpm).value);
}

void test_unrelated_frame_is_ignored_without_invalid_count() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);
    manager.setSource(DataSource::Ecumaster);

    const CanFrame unrelated = makeFrame(0x123, {1,2,3,4,5,6,7,8});
    TEST_ASSERT_FALSE(manager.handleCanFrame(unrelated, 3000U));
    TEST_ASSERT_EQUAL_UINT32(0U, manager.invalidFrameCount());
    TEST_ASSERT_FALSE(manager.canOnline());
}

void test_bad_dlc_inside_ecumaster_range_counts_invalid() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);
    manager.setSource(DataSource::Ecumaster);

    const CanFrame malformed = makeFrame(0x600, {1,2,3,4,5,6,7});
    TEST_ASSERT_FALSE(manager.handleCanFrame(malformed, 3000U));
    TEST_ASSERT_EQUAL_UINT32(1U, manager.invalidFrameCount());
    TEST_ASSERT_FALSE(manager.canOnline());
}

void test_timeout_invalidates_stale_can_values() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);
    manager.setSource(DataSource::Ecumaster);
    manager.setTimeoutMs(500U);

    const CanFrame frame = makeFrame(0x600, {0xD2,0x04, 0,0, 0,0, 0,0});
    TEST_ASSERT_TRUE(manager.handleCanFrame(frame, 1000U));
    TEST_ASSERT_TRUE(registry.value(ParameterId::Rpm).valid);

    manager.update(1500U);
    TEST_ASSERT_TRUE(manager.canOnline());
    TEST_ASSERT_TRUE(registry.value(ParameterId::Rpm).valid);

    manager.update(1501U);
    TEST_ASSERT_FALSE(manager.canOnline());
    TEST_ASSERT_FALSE(registry.value(ParameterId::Rpm).valid);
}

void test_source_switch_invalidates_previous_data_and_mock_recovers() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);
    manager.update(100U);
    TEST_ASSERT_TRUE(registry.value(ParameterId::Rpm).valid);

    manager.setSource(DataSource::Ecumaster);
    TEST_ASSERT_FALSE(registry.value(ParameterId::Rpm).valid);
    TEST_ASSERT_FALSE(manager.canOnline());

    manager.setSource(DataSource::Mock);
    manager.update(200U);
    TEST_ASSERT_TRUE(registry.value(ParameterId::Rpm).valid);
}

void test_rusefi_is_reserved_but_not_wired_in_phase_a() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);
    manager.setSource(DataSource::Rusefi);

    manager.update(1000U);
    TEST_ASSERT_FALSE(manager.canOnline());
    TEST_ASSERT_FALSE(registry.value(ParameterId::Rpm).valid);
}

void test_ecumaster_base_id_can_be_changed_through_manager() {
    ParameterRegistry registry;
    DataSourceManager manager(registry);
    manager.setSource(DataSource::Ecumaster);
    manager.setEcumasterBaseId(0x700);

    const CanFrame old_id = makeFrame(0x600, {0xD2,0x04,0,0,0,0,0,0});
    const CanFrame new_id = makeFrame(0x700, {0xD2,0x04,0,0,0,0,0,0});
    TEST_ASSERT_FALSE(manager.handleCanFrame(old_id, 100U));
    TEST_ASSERT_TRUE(manager.handleCanFrame(new_id, 101U));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1234.0f, registry.value(ParameterId::Rpm).value);
}
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_mock_source_publishes_registry_values);
    RUN_TEST(test_ecumaster_valid_frame_sets_online_and_decodes);
    RUN_TEST(test_unrelated_frame_is_ignored_without_invalid_count);
    RUN_TEST(test_bad_dlc_inside_ecumaster_range_counts_invalid);
    RUN_TEST(test_timeout_invalidates_stale_can_values);
    RUN_TEST(test_source_switch_invalidates_previous_data_and_mock_recovers);
    RUN_TEST(test_rusefi_is_reserved_but_not_wired_in_phase_a);
    RUN_TEST(test_ecumaster_base_id_can_be_changed_through_manager);
    return UNITY_END();
}
